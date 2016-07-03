// 3D subtitle rendering tool
// Copyright 2016 Vyacheslav Napadovsky.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Filter.SRTRenderer.hpp"
#include "Tools.Depth.hpp"
#include <common.h>

using namespace Tools::AviSynth;
using namespace Tools::Depth;

namespace Filter {
	namespace SRTRenderer {

		// input & output

		std::istream& operator>>(std::istream& in, subtitles_map_t& map) {
			map.clear();
			int lastnumber = 0;
			while (!in.eof()) {
				int number, start_at, length, depth;
				in >> number >> start_at >> length >> depth;
				if (in.fail()) {
					in.clear();
					in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					continue;
				}
				if (number != ++lastnumber)
					fprintf(stderr, ".depths loading: subtitle number mismatch: %d but %d expected. Ignoring.\n", number, lastnumber);
				map.insert(std::make_pair(start_at, subtitle_desc_t(length, depth)));
			}
			return in;
		}

		std::ostream& operator<<(std::ostream& out, const subtitles_map_t& map) {
			int number = 0;
			for (subtitles_map_t::const_iterator it = map.begin(); it != map.end(); ++it)
				out << ++number << ' ' << it->first << ' ' << it->second.length << ' ' << it->second.depth << std::endl;
			return out;
		}





		// Overlaying subtitles considering alpha channel

		cSubOverlay::cSubOverlay(IScriptEnvironment* env, PClip source, PClip text, int depth) :
			GenericVideoFilter(source),
			m_delta(depth, 0),
			text(text)
		{
			if (!vi.IsRGB32())
				env->ThrowError("plugin supports RGB32 input only");
			tvi = text->GetVideoInfo();
			if (vi.pixel_type != tvi.pixel_type || vi.num_frames != tvi.num_frames)
				env->ThrowError("source video parameters for all input videos must be the same");
		}

		PVideoFrame WINAPI cSubOverlay::GetFrame(int n, IScriptEnvironment* env) {
			Frame fsrc(env, child, n);
			Frame ftext(env, text, n);
			Frame fdst(env, vi);

#pragma omp parallel for schedule(guided)
			for (int y = 0; y < fsrc.height(); ++y) {
				for (int x = 0; x < fsrc.width(); ++x) {
					const Frame::Pixel &px_src = fsrc.read(x, y);
					Frame::Pixel &px_dst = fdst.write(x, y);
					Point text_pos = Point(x, y) - m_delta;
					if (text_pos.IsInside(0, 0, tvi.width - 1, tvi.height - 1)) {
						const Frame::Pixel &px_text = ftext.read(text_pos.x, text_pos.y);
						using Pixel = Frame::Pixel;
						px_dst.r = Pixel::ColorLimits(((int)px_text.r * px_text.a + (int)px_src.r * (255 - px_text.a)) / 255);
						px_dst.g = Pixel::ColorLimits(((int)px_text.g * px_text.a + (int)px_src.g * (255 - px_text.a)) / 255);
						px_dst.b = Pixel::ColorLimits(((int)px_text.b * px_text.a + (int)px_src.b * (255 - px_text.a)) / 255);
					}
					else {
						px_dst = px_src;
					}
					px_dst.a = 255;
				}
			}
			return fdst;
		}
		
		
		
		
		
		Renderer::Renderer(IScriptEnvironment* env, PClip source, const std::string& srtfile, const std::string& fnMasksub) :
			GenericVideoFilter(source),
			env(env),
			m_srtFile(srtfile),
			m_depthsLoaded(false)
		{
			if (!vi.IsRGB32())
				throw std::runtime_error("plugin supports RGB32 input only");

			// Try to load .depths file
			m_depthsFile = srtfile + ".depths";
			{
				std::ifstream f(m_depthsFile.c_str());
				if (f.is_open()) {
					f >> subtitles;
					m_depthsLoaded = true;
				}
			}

			// Create subtitle mask stream
			ClipSplit(env, source, cLeft, cRight);
			const VideoInfo &leftvi = cLeft->GetVideoInfo();
			LPCSTR lpMaskSub = fnMasksub.empty() ? "MaskSub" : fnMasksub.c_str();
			cSubtitle = AvsCall(env, lpMaskSub,
				srtfile.c_str(), leftvi.width, leftvi.height,
				(float)leftvi.fps_numerator / leftvi.fps_denominator, leftvi.num_frames).AsClip();

			ofLeftHolder = ofLeft = new cSubOverlay(env, cLeft, cSubtitle, 0);
			ofRightHolder = ofRight = new cSubOverlay(env, cRight, cSubtitle, 0);
		}

		void Renderer::CalculateDepth(bool showProgress) {
			subtitles.clear();

			auto script = std::make_shared<Tools::Lua::Script>("", m_srtFile, vi);
			Tools::Depth::Estimator depth(script);

			Frame sub, subPrev;

			bool found = false;
			int found_at, n;
			for (n = 0; n < vi.num_frames; ++n) {
				subPrev = sub;

				Frame fleft(env, cLeft, n);
				Frame fright(env, cRight, n);
				sub = Frame(env, cSubtitle, n);
				sub.EnableWriting();

				if (showProgress) {
					printf("\r[%5.1f%%] Subtitle depth detection ( %d/%d frames processed )",
						(float)((n + 1) * 100) / vi.num_frames, n + 1, vi.num_frames);
				}

				if (IsSubtitleExists(sub, subtitle_monitored_by_alpha)) {
					if (found && !subPrev.empty() && subPrev != sub) {
						// subtitle ended
						int value;
						if (depth.CalculateDepth(value))
							addSubtitle(found_at, n - found_at, value);
						found = false;
					}
					// found == true if same subtitle exists at previous frame
					if (!found) {
						// new started
						depth.SetMasks(sub, subtitle_monitored_by_alpha);
						found_at = n;
					}
					// continuing to monitor subtitle
					depth.ProcessFrame(fleft, fright, n);
					found = true;
				}
				else if (found) {
					// subtitle ended
					int value;
					if (depth.CalculateDepth(value))
						addSubtitle(found_at, n - found_at, value);
					found = false;
				}
			}
			if (found) {
				// subtitle ended
				int value;
				if (depth.CalculateDepth(value))
					addSubtitle(found_at, n - found_at, value);
				found = false;
			}

			std::ofstream f(m_depthsFile.c_str());
			if (f.is_open()) {
				f << "# <subno> <frame> <length> <depth*2>" << std::endl;
				f << subtitles;
				f << "# total frames: " << vi.num_frames << std::endl;
				f << "# hstack resolution: " << vi.width << '*' << vi.height << std::endl;
				f << "# resolution: " << vi.width / 2 << '*' << vi.height << std::endl;
				f << "# fps: " << (float)vi.fps_numerator / vi.fps_denominator << " (i.e. " << vi.fps_numerator << '/' << vi.fps_denominator << ")" << std::endl;
				f << "# sub file: " << m_srtFile << std::endl;
			}
			else
				throw std::runtime_error("Can't open .depths file \"" + m_depthsFile + "\" for write");
		}


		PVideoFrame WINAPI Renderer::GetFrame(int n, IScriptEnvironment* env) {
			//if (!m_depthsLoaded)
			//	env->ThrowError(".depths file (%s) was not created. Thus no subtitles can be loaded", m_depthsFile.c_str());

			Frame fLeft(env, cLeft, n);
			Frame fRight(env, cRight, n);
			Frame fSubtitle(env, cSubtitle, n);

			subtitles_map_t::const_iterator it = subtitles.lower_bound(n);
			if (subtitles.empty()) {
			nosub:
				return FrameStack(env, fLeft.GetVideoInfo(), fLeft, fRight, true);
			}
			if (it == subtitles.end())
				--it;
			if (it->first > n) {
				if (it == subtitles.begin())
					goto nosub;
				else
					--it;
			}
			const subtitle_desc_t& sub_desc = it->second;
			if (n >= it->first + sub_desc.length)
				goto nosub;

			PClip fhLeft = new FrameHolder(vi, fLeft);
			PClip fhRight = new FrameHolder(vi, fRight);
			PClip fhSubtitle = new FrameHolder(vi, fSubtitle);

			ofLeft->setDelta(Point(DIV_AND_ROUND(sub_desc.depth, 2), 0));
			ofRight->setDelta(Point(DIV_AND_ROUND(-sub_desc.depth, 2), 0));
			PVideoFrame left_overlay = ofLeft->GetFrame(n, env);
			PVideoFrame right_overlay = ofRight->GetFrame(n, env);;
			return FrameStack(env, fLeft.GetVideoInfo(), left_overlay, right_overlay, true);
		}



		AvsParams Renderer::CalculateParams = "[hstack]c[srtfile]s[masksub]s[progress]b";

		AVSValue Renderer::Calculate(AVSValue args, void* user_data, IScriptEnvironment* env) {
			try {
				auto filter = new Renderer(env, args[0].AsClip(), args[1].AsString(), args[2].AsString(""));
				AVSValue holder(filter);
				filter->CalculateDepth(args[3].AsBool(false));
			}
			catch (const std::runtime_error& err) {
				env->ThrowError("%s", err.what());
			}
			return AVSValue();
		}


		AvsParams Renderer::RenderParams = "[hstack]c[srtfile]s[masksub]s";

		AVSValue Renderer::Render(AVSValue args, void* user_data, IScriptEnvironment* env) {
			try {
				return new Renderer(env, args[0].AsClip(), args[1].AsString(), args[2].AsString(""));
			}
			catch (const std::runtime_error& err) {
				env->ThrowError("%s", err.what());
			}
			return AVSValue();
		}


	}
}
