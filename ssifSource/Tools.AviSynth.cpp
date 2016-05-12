#include "stdafx.h"
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

	}
}
