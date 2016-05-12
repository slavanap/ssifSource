#pragma once

namespace Tools {
	namespace AviSynth {

		// stub for AviSynth source video filters that doesn't support audio playback
		class SourceFilterStub : public IClip {
		public:
			SourceFilterStub(const VideoInfo& vi) :
				vi(vi)
			{
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
			FrameHolder(const VideoInfo& vi, PVideoFrame& vf) :
				SourceFilterStub(vi), vf(vf)
			{
			}
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override {
				return vf;
			}
		private:
			PVideoFrame vf;
		};

		PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal);
		PVideoFrame FrameStack(IScriptEnvironment* env, VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal);

	}
}
