//
// AviSynth useful C++ wrappers
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
#include <thirdparty/AviSynth/avisynth.h>
#include <string>

typedef const LPCSTR AvsParams;

namespace Tools {
	namespace AviSynth {

		// template that allows to call AviSynth functions easily
		struct AvsNamedArg {
			AVSValue value;
			const char *name;
			AvsNamedArg(const char* name, const AVSValue& value) : name(name), value(value) { }
			operator AVSValue() const { return value; }

			template<typename T> static const char* GetName(const T& v) {
				return nullptr;
			}
		};
		template<> inline const char* AvsNamedArg::GetName(const AvsNamedArg& v) {
			return v.name;
		}

		template<typename ... Args>
		AVSValue AvsCall(IScriptEnvironment* env, const char* name, const Args& ... args) {
			const char* arg_names[sizeof...(Args)] = { AvsNamedArg::GetName(args) ... };
			AVSValue arg_values[sizeof...(Args)] = { args ... };
			return env->Invoke(name, AVSValue(arg_values, sizeof...(Args)), arg_names);
		}
		template<>
		inline AVSValue AvsCall(IScriptEnvironment* env, const char* name) {
			return env->Invoke(name, AVSValue(0, 0));
		}

		// stub for AviSynth source video filters that doesn't support audio playback
		class SourceFilterStub : public IClip {
		public:
			SourceFilterStub() {
				memset(&vi, 0, sizeof(vi));
			}
			bool WINAPI GetParity(int n) override {
				return false;
			}
			void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override {
				// empty
			}
			const VideoInfo& WINAPI GetVideoInfo() override {
				return vi;
			}

#if defined __AVISYNTH_H__
			void WINAPI SetCacheHints(int cachehints, int frame_range) override {
				// empty
			}
#elif defined __AVISYNTH_6_H__
			int WINAPI SetCacheHints(int cachehints, int frame_range) override {
				// empty
				return 0;
			}
#endif

		protected:
			VideoInfo vi;
		};


		// class for holding one frame
		class FrameHolder : public SourceFilterStub {
		public:
			FrameHolder(const VideoInfo& vi, PVideoFrame vf) :
				vf(vf)
			{
				this->vi = vi;
			}
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override {
				return vf;
			}
		private:
			PVideoFrame vf;
		};


		inline PVideoFrame ClipToFrame(IScriptEnvironment* env, PClip clip) {
			return clip->GetFrame(0, env);
		}

		PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal);
		PVideoFrame FrameStack(IScriptEnvironment* env, const VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal);
		PClip ConvertToRGB32(IScriptEnvironment* env, PClip src);
		void ClipSplit(IScriptEnvironment* env, PClip source, PClip& left, PClip& right);
		void CheckVideoInfo(IScriptEnvironment* env, const VideoInfo& svi, const VideoInfo& tvi);

		template<typename T>
		inline void AddFilter(IScriptEnvironment* env) {
			env->AddFunction(T::FilterName, T::CreateParams, T::Create, nullptr);
		}

	}
}
