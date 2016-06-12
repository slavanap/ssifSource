#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {
	namespace Pipe {

		extern AvsParams CreateReadPipeParams;
		AVSValue __cdecl CreateReadPipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		extern AvsParams CreateWritePipeParams;
		AVSValue __cdecl CreateWritePipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		extern AvsParams DestroyPipeParams;
		AVSValue __cdecl DestroyPipe(AVSValue args, void* user_data, IScriptEnvironment* env);

		class PipeWriter : public GenericVideoFilter {
		public:
			PipeWriter(IScriptEnvironment* env, PClip clip, const std::string& pipe_name);
			PipeWriter(IScriptEnvironment* env, PClip clip, HANDLE hPipe);
			~PipeWriter();
			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			// AviSynth functions
			static AvsParams CreateParams;
			static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
			static AvsParams CreateForHandleParams;
			static AVSValue __cdecl CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env);
		private:
			HANDLE hPipe;
			bool flagClose;
			void WriteVideoInfoToPipe(IScriptEnvironment* env);
		};

		class PipeReader : public Tools::AviSynth::SourceFilterStub {
		public:
			PipeReader(IScriptEnvironment* env, const std::string& pipe_name);
			PipeReader(IScriptEnvironment* env, HANDLE hPipe);
			~PipeReader();

			PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

			// AviSynth functions
			static AvsParams CreateParams;
			static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
			static AvsParams CreateForHandleParams;
			static AVSValue __cdecl CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env);
			
		private:
			HANDLE hPipe;
			bool flagClose;

			void ReadVideoInfoFromPipe(IScriptEnvironment* env);
		};

	}
}
