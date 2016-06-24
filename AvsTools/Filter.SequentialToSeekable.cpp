// Converts any sequesntial AviSynth plugin to seekable
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
