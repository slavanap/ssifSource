// AviSynth useful C++ wrappers
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
#include "Tools.AviSynth.hpp"
#include "common.h"

namespace Tools {
	namespace AviSynth {

		PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal) {
			return AvsCall(env, horizontal ? "StackHorizontal" : "StackVertical", a, b).AsClip();
		}

		PVideoFrame FrameStack(IScriptEnvironment* env, const VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal) {
			return ClipStack(env, new FrameHolder(vi, a), new FrameHolder(vi, b), horizontal)->GetFrame(0, env);
		}

		PClip ConvertToRGB32(IScriptEnvironment* env, PClip src) {
			if (src->GetVideoInfo().IsRGB32())
				return src;
			return AvsCall(env, "ConvertToRGB32", src).AsClip();
		}

		void ClipSplit(IScriptEnvironment* env, PClip source, PClip& left, PClip& right) {
			const char* arg_names[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
			const VideoInfo &vi = source->GetVideoInfo();
			AVSValue args[5] = { source, 0, 0, vi.width / 2, vi.height };
			left = env->Invoke("crop", AVSValue(args, ARRAY_SIZE(args)), arg_names).AsClip();
			args[1] = vi.width / 2;
			right = env->Invoke("crop", AVSValue(args, ARRAY_SIZE(args)), arg_names).AsClip();
			CheckVideoInfo(env, left->GetVideoInfo(), right->GetVideoInfo());
		}

		void CheckVideoInfo(IScriptEnvironment* env, const VideoInfo& svi, const VideoInfo& tvi) {
			bool success =
				svi.width == tvi.width &&
				svi.height == tvi.height &&
				svi.pixel_type == tvi.pixel_type &&
				svi.num_frames == tvi.num_frames;
			if (!success)
				env->ThrowError("source video parameters for all input videos must be the same");
		}

	}
}
