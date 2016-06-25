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
#include "Tools.Lua.hpp"
#include <Tools.Motion.hpp>

namespace Tools {
	namespace Depth {

		using AviSynth::Frame;
		using Motion::Map;
		using Lua::Script;

		inline bool subtitle_monitored_by_alpha(const Frame::Pixel& p) {
			return p.a != 0;
		}

		inline bool subtitle_monitored_by_color(const Frame::Pixel& p) {
			return p.r != 0 && p.g != 0 && p.b != 0;
		}

		template<typename F>
		bool IsSubtitleExists(const Frame& sub, F is_monitored) {
			for (auto it = sub.cbegin(); it != sub.cend(); ++it)
				if (is_monitored(*it))
					return true;
			return false;
		}

		class Estimator {
		public:
			Estimator(std::shared_ptr<Script> script);

			template<typename F>
			void SetMasks(const Frame& sub, F is_monitored) {
				SetMasksSize(sub.width(), sub.height());
				for (auto it = sub.cbegin(); it != sub.cend(); ++it) {
					bool m = is_monitored(*it);
					subtitle_mask(it.get_x(), it.get_y()) = m;
					auto &v = mv_mask(it.get_x() / Tools::Motion::BLOCK_SIZE, it.get_y() / Tools::Motion::BLOCK_SIZE);
					v = v || m;
				}
			}

			void SetMasksSize(int x, int y);
			void ProcessFrame(Frame& left, Frame& right, int frame_num);

			bool CalculateDepth(int& depth) {
				bool ret = m_script->CalculateForSubtitle(startFrame, lastFrame - startFrame + 1, depth);
				startFrame = -1;
				return ret;
			}

			Array2D<char> mv_mask, subtitle_mask;

		private:
			std::shared_ptr<Script> m_script;
			int startFrame, lastFrame;
			Motion::Map mv_map;

		};

	}
}
