#pragma once
#include <AviSynth/avisynth.h>
#include <string>

typedef const LPCSTR AvsParams;

namespace Tools {
	namespace AviSynth {

		// template that allows to call AviSynth functions easily
		struct AvsNamedArg {
			AVSValue value;
			LPCSTR name;
			AvsNamedArg(LPCSTR name, AVSValue value) : name(name), value(value) { }
			operator AVSValue() { return value; }

			template<typename T> static LPCSTR GetName(T v) {
				return nullptr;
			}
			template<> static LPCSTR GetName(AvsNamedArg v) {
				return v.name;
			}
		};

		template<typename ... Args>
		AVSValue AvsCall(IScriptEnvironment* env, const char* name, Args ... args) {
			LPCSTR arg_names[sizeof...(Args)] = { AvsNamedArg::GetName(args) ... };
			AVSValue args[sizeof...(Args)] = { args ... };
			return env->Invoke(name, AVSValue(args, sizeof...(Args)), arg_names);
		}


		// stub for AviSynth source video filters that doesn't support audio playback
		class SourceFilterStub : public IClip {
		public:
			SourceFilterStub() {
				// empty
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
			void WINAPI SetCacheHints(int cachehints, int frame_range) override {
				// empty
			}
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
		PVideoFrame FrameStack(IScriptEnvironment* env, VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal);
		PClip ConvertToRGB32(IScriptEnvironment* env, PClip src);
		void ClipSplit(IScriptEnvironment* env, PClip source, PClip& left, PClip& right);
		void CheckVideoInfo(IScriptEnvironment* env, const VideoInfo& svi, const VideoInfo& tvi);

	}
}
