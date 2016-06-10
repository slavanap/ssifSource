#pragma once

namespace Filter {
	namespace Pipe {

		extern LPCSTR CreateReadPipeParams;
		AVSValue __cdecl CreateReadPipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		extern LPCSTR CreateWritePipeParams;
		AVSValue __cdecl CreateWritePipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		extern LPCSTR DestroyPipeParams;
		AVSValue __cdecl DestroyPipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		class PipeWriter : public GenericVideoFilter {
		public:
			PipeWriter(IScriptEnvironment* env, PClip clip, const std::string& pipe_name);
			PipeWriter(IScriptEnvironment* env, PClip clip, HANDLE hPipe);
			~PipeWriter();
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			// AviSynth functions
			static LPCSTR CreateParams;
			static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
			static LPCSTR CreateForHandleParams;
			static AVSValue __cdecl CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env);
		private:
			HANDLE hPipe;
			bool flagClose;
			void WriteVideoInfoToPipe(IScriptEnvironment* env);
		};

		class PipeReader : public IClip {
		public:
			PipeReader(IScriptEnvironment* env, const std::string& pipe_name);
			PipeReader(IScriptEnvironment* env, HANDLE hPipe);
			~PipeReader();

			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			// AviSynth functions
			static LPCSTR CreateParams;
			static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
			static LPCSTR CreateForHandleParams;
			static AVSValue __cdecl CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env);
			
			// stubs
			bool WINAPI GetParity(int n) { return false; }
			void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { }
			const VideoInfo& WINAPI GetVideoInfo() { return vi; }
			void WINAPI SetCacheHints(int cachehints, int frame_range) { }

		private:
			HANDLE hPipe;
			bool flagClose;
			VideoInfo vi;

			void ReadVideoInfoFromPipe(IScriptEnvironment* env);
		};

	}
}
