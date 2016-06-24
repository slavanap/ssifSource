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

#pragma once
#include "Tools.Motion.hpp"

namespace Filter {
	namespace SRTRenderer {

		using Tools::Point;

		struct subtitle_desc_t {
			int length;
			int depth;
			subtitle_desc_t() {}
			subtitle_desc_t(int length, int depth) : length(length), depth(depth) { }
		};
		typedef std::map<int, subtitle_desc_t> subtitles_map_t;

		std::istream& operator>>(std::istream& in, subtitles_map_t& map);
		std::ostream& operator<<(std::ostream& out, const subtitles_map_t& map);


		class cSubOverlay : public GenericVideoFilter {
		public:
			cSubOverlay(IScriptEnvironment* env, PClip source, PClip text, int depth = 0);
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			Point delta() const { return m_delta; }
			void setDelta(const Point& delta) { m_delta = delta; }

		private:
			Point m_delta;
			PClip text;
			VideoInfo tvi;
		};


		class Renderer : public GenericVideoFilter {
		public:
			Renderer(IScriptEnvironment* env, PClip source, const std::string& srtfile, const std::string& fnMasksub);
			void CalculateDepth(bool showProgress);
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			static AvsParams CalculateParams;
			static AVSValue __cdecl Calculate(AVSValue args, void* user_data, IScriptEnvironment* env);

			static AvsParams RenderParams;
			static AVSValue __cdecl Render(AVSValue args, void* user_data, IScriptEnvironment* env);

		private:
			IScriptEnvironment *env;
			std::string m_srtFile, m_depthsFile;
			bool m_depthsLoaded;
			subtitles_map_t subtitles;

			PClip cLeft, cRight, cSubtitle;
			cSubOverlay *ofLeft, *ofRight;
			PClip ofLeftHolder, ofRightHolder;

			void addSubtitle(int start_at, int length, int depth) {
				subtitles.insert(std::make_pair(start_at, subtitle_desc_t(length, depth)));
			}
		};

	}
}
