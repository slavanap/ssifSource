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
#include "Tools.Depth.hpp"
#include <common.h>

using namespace Tools::Motion;

namespace Tools {
	namespace Depth {

		Estimator::Estimator(std::shared_ptr<Lua::Script> script) :
			m_script(script)
		{
			startFrame = -1;
			lastFrame = -1;
		}

		void Estimator::SetMasksSize(int x, int y) {
			subtitle_mask.resize(0, 0);
			subtitle_mask.resize(x, y, false);

			mv_mask.resize(0, 0);
			mv_mask.resize(DIV_AND_ROUND(x, BLOCK_SIZE), DIV_AND_ROUND(y, BLOCK_SIZE), 0);

			mv_map.resize(0, 0);
			mv_map.resize(DIV_AND_ROUND(x, BLOCK_SIZE), DIV_AND_ROUND(y, BLOCK_SIZE));
		}

		void Estimator::ProcessFrame(const Frame& left, const Frame& right, int frame_num) {
			if (startFrame == -1)
				startFrame = frame_num;
			lastFrame = frame_num;

			assert(left.width() == right.width() && left.height() == right.height());
			assert(left.width() == subtitle_mask.width() && left.height() == subtitle_mask.height());
			assert(mv_mask.width() == mv_map.width() && mv_mask.height() == mv_map.height());

			Motion::Estimate(left, right, mv_map, mv_mask);

			Script::depth_list_t queue;
			for (int i = 0; i < (int)subtitle_mask.size(); ++i) {
				if (subtitle_mask[i]) {
					int x = i % subtitle_mask.width(), y = i / subtitle_mask.width();
					auto &vec = mv_map(x / BLOCK_SIZE, y / BLOCK_SIZE);
					queue.emplace_back(std::make_pair(-vec.dx, GetRealConfidence(vec)));
				}
			}
			m_script->CalculateForFrame(frame_num, queue);
		}

	}
}
