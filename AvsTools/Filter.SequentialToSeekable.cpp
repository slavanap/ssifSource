#include "stdafx.h"
#include "Filter.SequentialToSeekable.hpp"

namespace Filter {

	using Tools::AviSynth::AvsCall;

	// Plugin that converts sequentially encoded streams to rewindable streams

	PClip SequentialToSeekable::eval(const char* command) {
		return AvsCall(env, "eval", command, "Rewind support plugin").AsClip();
	}

	SequentialToSeekable::SequentialToSeekable(IScriptEnvironment* env, const char* command) :
		env(env), command(command)
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
			clip = nullptr;
			clip = eval(command.c_str());
			for (int i = 0; i <= n; ++i)
				frame = clip->GetFrame(i, env);
		}
		last_index = n;
		return frame;
	}

	AvsParams SequentialToSeekable::CreateParams = "[command]s";

	AVSValue __cdecl SequentialToSeekable::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return new SequentialToSeekable(env, args[0].AsString());
	}

}
