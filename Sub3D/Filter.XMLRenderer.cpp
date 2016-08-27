//
// 3D subtitle rendering tool
// Copyright 2016 Vyacheslav Napadovsky.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include "stdafx.h"
#include "Filter.XMLRenderer.hpp"
#include <Tools.WinApi.hpp>
#include <common.h>
#include "Tools.Depth.hpp"

using namespace Tools;
using namespace Tools::AviSynth;
using namespace Tools::WinApi;
using namespace Tools::Depth;
using namespace Tools::Motion;

namespace Filter {

	int XMLRenderer::time2framenum(const char* time) const {
		int h, m, s, f;
		if (sscanf_s(time, "%2d:%2d:%2d:%2d", &h, &m, &s, &f) != 4)
			throw std::runtime_error("invalid time format in xml file");
		s += m * 60 + h * 3600;
		f += (s * vi.fps_numerator) / vi.fps_denominator;
		return f;
	}

	int XMLRenderer::str2number(tinyxml2::XMLElement* elem, const char* attr) const {
		int i;
		if (elem->QueryIntAttribute(attr, &i) != tinyxml2::XML_NO_ERROR)
			throw std::runtime_error("invalid number format in xml file");
		return i;
	}





	XMLRenderer::XMLRenderer(IScriptEnvironment* env, PClip source, const std::string& xmlfile) :
		GenericVideoFilter(source),
		env(env),
		m_forcedOnly(false),
		m_recalcAllValues(false),
		m_xmlfile(xmlfile)
	{
		if (!vi.IsRGB32())
			throw std::runtime_error("plugin supports only RGB32 input");

		graphics_path = ExtractFileDir(xmlfile);
		std::string xmlnameonly = ExtractFileName(xmlfile);

		if (xml.LoadFile(xmlfile.c_str()) != tinyxml2::XML_NO_ERROR)
			throw std::runtime_error("Can't open xml file \"" + xmlfile + "\"");
		for (tinyxml2::XMLElement *xml_event = xml.FirstChildElement("BDN")->FirstChildElement("Events")->FirstChildElement("Event"); xml_event != nullptr; xml_event = xml_event->NextSiblingElement("Event")) {
			xEvent event;
			event.in_frame = time2framenum(xml_event->Attribute("InTC"));
			event.out_frame = time2framenum(xml_event->Attribute("OutTC"));
			event.forced = !strcasecmp(xml_event->Attribute("Forced"), "False");

			for (tinyxml2::XMLElement *xml_graphic = xml_event->FirstChildElement("Graphic"); xml_graphic != nullptr; xml_graphic = xml_graphic->NextSiblingElement("Graphic")) {
				xGraphic graphic;
				graphic.x = str2number(xml_graphic, "X");
				graphic.y = str2number(xml_graphic, "Y");
				graphic.width = str2number(xml_graphic, "Width");
				graphic.height = str2number(xml_graphic, "Height");
				if (graphic.width <= 0 || graphic.height <= 0) {
					fprintf(stderr, "WARNING (%s): skipping subtitle InTC=%s, OutTC=%s, Forced=%s, X=%s, Y=%s, Width=%s, Height=%s\n",
						xmlnameonly.c_str(),
						xml_event->Attribute("InTC"), xml_event->Attribute("OutTC"),
						xml_event->Attribute("Forced"),
						xml_graphic->Attribute("X"), xml_graphic->Attribute("Y"),
						xml_graphic->Attribute("Width"), xml_graphic->Attribute("Height"));
					goto skip;
				}
				if (xml_graphic->QueryIntAttribute("Depth", &graphic.depth) == tinyxml2::XML_NO_ATTRIBUTE)
					graphic.depth = DEPTH_UNDEF;
				graphic.filename = xml_graphic->GetText();
				graphic.xml_elem = xml_graphic;
				event.graphics.push_back(graphic);
			}
			event.xml_elem = xml_event;
			subtitles.insert(std::make_pair(event.in_frame, event));
		skip:
			;
		}

		loaded_image_it = subtitles.end();
	}


	// CALCULATE

	void XMLRenderer::CalculateDepth() {
		PClip cleft, cright;
		ClipSplit(env, child, cleft, cright);

		auto script = std::make_shared<Tools::Lua::Script>(m_xmlfile, "", vi);
		Tools::Depth::Estimator depth(script);

		for (std::map<int, xEvent>::iterator sub_it = subtitles.begin(); sub_it != subtitles.end(); ++sub_it) {
			xEvent &event = sub_it->second;
			xGraphic &graphic = event.graphics[0];
			if (!m_recalcAllValues && graphic.depth != DEPTH_UNDEF)
				continue;

			{
				std::string filename = this->graphics_path + graphic.filename;
				PClip image_clip = AvsCall(env, "imagesource", filename.c_str(),
					AvsNamedArg("pixel_type", "RGB32"), AvsNamedArg("end", this->vi.num_frames)).AsClip();
				const VideoInfo &image_vi = image_clip->GetVideoInfo();
				graphic.width = image_vi.width;
				graphic.height = image_vi.height;

				depth.SetMasksSize(vi.width / 2, vi.height);
				assert(sizeof(Frame::Pixel) == 4);

				Frame frame(env, image_clip, 0);
				for (auto it = frame.cbegin(); it != frame.cend(); ++it) {
					Point pos(graphic.x + it.get_x(), graphic.y + it.get_y());
					if (pos.IsInside(0, 0, depth.subtitle_mask.width() - 1, depth.subtitle_mask.height() - 1)) {
						if (bool monitored = subtitle_monitored_by_alpha(*it)) {
							depth.subtitle_mask(pos.x, pos.y) = monitored;
							auto &v = depth.mv_mask(pos.x / BLOCK_SIZE, pos.y / BLOCK_SIZE);
							v = v || monitored;
						}
					}
				}
			}

			for (int frame_no = event.in_frame; frame_no < event.out_frame; ++frame_no)
				depth.ProcessFrame(Frame(env, cleft, frame_no), Frame(env, cright, frame_no), frame_no);

			int res_depth = 0;
			depth.CalculateDepth(res_depth);
			graphic.depth = res_depth;

			graphic.xml_elem->SetAttribute("Width", graphic.width);
			graphic.xml_elem->SetAttribute("Height", graphic.height);
			graphic.xml_elem->SetAttribute("Depth", graphic.depth);
		}
		xml.SaveFile(m_xmlfile.c_str());
	}

	PVideoFrame WINAPI XMLRenderer::GetFrame(int n, IScriptEnvironment* env) {
		PVideoFrame frame = child->GetFrame(n, env);

		std::map<int, xEvent>::const_iterator it = subtitles.lower_bound(n);
		if (subtitles.empty()) {
		nosub:
			return frame;
		}
		if (it == subtitles.end())
			--it;
		if (it->first > n) {
			if (it == subtitles.begin())
				goto nosub;
			else
				--it;
		}
		const xEvent &sub_desc = it->second;
		if (n >= sub_desc.out_frame || (m_forcedOnly && !sub_desc.forced))
			goto nosub;

		if (loaded_image_it != it) {
			overlayed_image = child;
			loaded_image_it = it;

			const xGraphic &graphic = sub_desc.graphics[0];
			std::string filename = this->graphics_path + graphic.filename;

			// load image
			PClip subtitle_image = AvsCall(env, "imagesource", filename.c_str(),
				AvsNamedArg("pixel_type", "RGB32"), AvsNamedArg("end", vi.num_frames)).AsClip();
			const VideoInfo &loaded_image_vi = subtitle_image->GetVideoInfo();
			if (loaded_image_vi.width != graphic.width || loaded_image_vi.height != graphic.height) {
				fprintf(stderr, "Graphic %s different scale loaded. Xml size (%d,%d), loaded size (%d,%d)\n",
					filename.c_str(), graphic.width, graphic.height, loaded_image_vi.width, loaded_image_vi.height);
			}
			PClip subtitle_image_alpha = AvsCall(env, "showalpha", subtitle_image).AsClip();

			// overlay left & right frames
			int depth = DIV_AND_ROUND(graphic.depth, 2);
			overlayed_image = AvsCall(env, "overlay", overlayed_image, subtitle_image,
				graphic.x + depth,
				graphic.y, subtitle_image_alpha).AsClip();
			overlayed_image = AvsCall(env, "overlay", overlayed_image, subtitle_image,
				graphic.x - depth + (vi.width / 2),
				graphic.y, subtitle_image_alpha).AsClip();
		}
		return overlayed_image->GetFrame(n, env);
	}



	AvsParams XMLRenderer::CalculateParams = "[hstack]c[xmlfile]s[RewriteAlreadyExistingValues]b";

	AVSValue XMLRenderer::Calculate(AVSValue args, void* user_data, IScriptEnvironment* env) {
		try {
			auto filter = new XMLRenderer(env, args[0].AsClip(), args[1].AsString());
			AVSValue holder(filter);
			filter->setRecalcAllValues(args[2].AsBool(true));
			filter->CalculateDepth();
		}
		catch (const std::runtime_error& err) {
			env->ThrowError("%s", err.what());
		}
		return AVSValue();
	}

	AvsParams XMLRenderer::RenderParams = "[hstack]c[xmlfile]s[RenderForcedCaptionsOnly]b";
	
	AVSValue XMLRenderer::Render(AVSValue args, void* user_data, IScriptEnvironment* env) {
		try {
			auto filter = new XMLRenderer(env, args[0].AsClip(), args[1].AsString());
			filter->setForcedOnly(args[2].AsBool(false));
			return filter;
		}
		catch (const std::runtime_error& err) {
			env->ThrowError("%s", err.what());
		}
		return AVSValue();
	}

}
