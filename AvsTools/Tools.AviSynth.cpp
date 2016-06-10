#include "stdafx.h"
#include "common.h"
#include "Tools.AviSynth.hpp"

namespace Tools {
	namespace AviSynth {

		PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal) {
			const char* arg_names[2] = { NULL, NULL };
			AVSValue args[2] = { a, b };
			return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args, 2), arg_names)).AsClip();
		}

		PVideoFrame FrameStack(IScriptEnvironment* env, VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal) {
			return ClipStack(env, new FrameHolder(vi, a), new FrameHolder(vi, b), horizontal)->GetFrame(0, env);
		}

		PClip ConvertToRGB32(IScriptEnvironment* env, PClip src) {
			if (src->GetVideoInfo().IsRGB32())
				return src;
			const char* arg_names[1] = { NULL };
			AVSValue args[1] = { src };
			return env->Invoke("ConvertToRGB32", AVSValue(args, 1), arg_names).AsClip();
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
