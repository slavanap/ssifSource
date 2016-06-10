#include "stdafx.h"
#include "Tools.AviSynth.hpp"
#include "Filter.SequentialToSeekable.hpp"

namespace Filter {

	// Plugin that converts sequentially encoded streams to rewindable streams

	PClip SequentialToSeekable::eval(const char* command) {
		const char* arg_names[2] = { nullptr, nullptr };
		AVSValue args[2] = { command, "Rewind support plugin" };
		return env->Invoke("eval", AVSValue(args, 2), arg_names).AsClip();
	}

	SequentialToSeekable::SequentialToSeekable(IScriptEnvironment* env, const char* command) :
		SourceFilterStub(VideoInfo()), env(env), command(command)
	{
		clip = eval(command);
		vi = clip->GetVideoInfo();
		last_index = -1;
	}

	PVideoFrame WINAPI SequentialToSeekable::GetFrame(int n, IScriptEnvironment* env) {
		if (n > last_index) {
			for (int i = last_index + 1; i <= n; ++i)
				frame = clip->GetFrame(i, env);
		}
		else if (n < last_index) {
			clip = NULL;
			clip = eval(command.c_str());
			for (int i = 0; i <= n; ++i)
				frame = clip->GetFrame(i, env);
		}
		last_index = n;
		return frame;
	}

	LPCSTR SequentialToSeekable::CreateParams = "[command]s";

	AVSValue __cdecl SequentialToSeekable::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return new SequentialToSeekable(env, args[0].AsString());
	}

}
