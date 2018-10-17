//
// Adjusted Color Difference filter
// Copyright 2017 Vyacheslav Napadovsky.
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
#include <memory>

#include "Filter.AdjustedColorDifference.hpp"

namespace Filter {

	using Tools::AviSynth::CheckVideoInfo;
	using Tools::AviSynth::Frame;

	AdjustedColorDifference::AdjustedColorDifference(IScriptEnvironment* env, PClip input, double factor, PClip subtrahend /* optional */) :
		GenericVideoFilter(input), m_factor(factor), m_subtrahend(subtrahend)
	{
		if (!vi.IsRGB32())
			env->ThrowError("plugin supports only RGB32 input");
		if (subtrahend != nullptr) {
			auto svi = subtrahend->GetVideoInfo();
			CheckVideoInfo(env, vi, svi);
		}
	}

	PVideoFrame AdjustedColorDifference::GetFrame(int n, IScriptEnvironment* env) {
		Frame finp(env, child, n);
		finp.EnableWriting();

		if (m_subtrahend != nullptr) {
			Frame fsub(env, m_subtrahend, n);
			for (int y = 0; y < finp.height(); ++y) {
				auto rinp = &finp.write_raw(0, y);
				auto rsub = &fsub.read_raw(0, y);
				for (int x = 0; x < finp.width(); ++x, ++rinp, ++rsub) {
					rinp->r = Frame::Pixel::ColorLimits((int)rinp->r - rsub->r + 128);
					rinp->g = Frame::Pixel::ColorLimits((int)rinp->g - rsub->g + 128);
					rinp->b = Frame::Pixel::ColorLimits((int)rinp->b - rsub->b + 128);
				}
			}
		}

		auto factor = m_factor;
		if (factor == 0) {
			// Calculate independent factor for each frame, if it is not specified.
			typedef int Histogram[Frame::ColorCount];

			Histogram hr, hg, hb;
			memset(&hr, 0, sizeof(hr));
			memset(&hg, 0, sizeof(hg));
			memset(&hb, 0, sizeof(hb));
			for (auto &pixel : finp) {
				hr[pixel.r]++;
				hg[pixel.g]++;
				hb[pixel.b]++;
			}

			int left_border, right_border;

			int max_val = finp.width() * finp.height();
			max_val /= 20;  // set border at 5% of pixels

			int thres_r = 0, thres_g = 0, thres_b = 0;
			for (left_border = 0; left_border < Frame::ColorCount / 2; ++left_border) {
				thres_r += hr[left_border];
				thres_g += hg[left_border];
				thres_b += hb[left_border];
				if (thres_r > max_val || thres_g > max_val || thres_b > max_val)
					break;
			}
			thres_r = thres_g = thres_b = 0;
			for (right_border = Frame::ColorCount - 1; right_border > Frame::ColorCount / 2; --right_border) {
				thres_r += hr[right_border];
				thres_g += hg[right_border];
				thres_b += hb[right_border];
				if (thres_r > max_val || thres_g > max_val || thres_b > max_val)
					break;
			}

			factor = 128.0 / std::max(128 - left_border, right_border - 128);
		}

		for (auto &pixel : finp) {
			pixel.r = (BYTE)(int) (((int)pixel.r - 128) * factor + 128.5);
			pixel.g = (BYTE)(int) (((int)pixel.g - 128) * factor + 128.5);
			pixel.b = (BYTE)(int) (((int)pixel.b - 128) * factor + 128.5);
		}
		return finp;
	}

	AvsParams AdjustedColorDifference::CreateParams =
		"[input]c"
		"[factor]f"
		"[sub]c";

	AVSValue AdjustedColorDifference::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		PClip subtrahend;
		if (args[2].IsClip())
			subtrahend = args[2].AsClip();
		return new AdjustedColorDifference(env, args[0].AsClip(), args[1].AsFloat(0), subtrahend);
	}

}
