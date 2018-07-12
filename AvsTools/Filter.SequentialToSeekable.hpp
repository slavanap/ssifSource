//
// Converts any sequesntial AviSynth plugin to seekable
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


#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {

	class SequentialToSeekable : public Tools::AviSynth::SourceFilterStub {
	public:
		SequentialToSeekable(IScriptEnvironment* env, const char* command);
		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

		bool WINAPI GetParity(int n) override {
			return clip->GetParity(n);
		}

		void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override {
			return clip->GetAudio(buf, start, count, env);
		}

#if defined __AVISYNTH_H__
		void WINAPI SetCacheHints(int cachehints, int frame_range) override {
			clip->SetCacheHints(cachehints, frame_range);
		}
#elif defined __AVISYNTH_6_H__
		int WINAPI SetCacheHints(int cachehints, int frame_range) override {
			return clip->SetCacheHints(cachehints, frame_range);
		}
#endif

		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	private:
		IScriptEnvironment *env;
		std::string command;
		PClip clip;
		PVideoFrame frame;
		int last_index;

		PClip eval(const char* command);
	};

}
