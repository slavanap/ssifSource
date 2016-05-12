#include "stdafx.h"
#include "Filter.Pipe.hpp"
#include "Filter.ssifSource.hpp"
#include "Tools.WinApi.hpp"
#include <ctime>

// DirectShow initialization
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            Tools::WinApi::hInstance = hModule;
			try {
				Tools::WinApi::AddCurrentPathToPathEnv();
				srand((unsigned int)time(NULL));
			}
			catch (const std::exception& ex) {
				MessageBoxA(HWND_DESKTOP, ex.what(), NULL, MB_ICONERROR | MB_OK);
				return FALSE;
			}
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

// AviSynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("CreateReadPipe", Filter::Pipe::CreateReadPipeParams, Filter::Pipe::CreateReadPipe, nullptr);
	env->AddFunction("CreateWritePipe", Filter::Pipe::CreateWritePipeParams, Filter::Pipe::CreateWritePipe, nullptr);
	env->AddFunction("PipeReader", Filter::Pipe::PipeReader::CreateParams, Filter::Pipe::PipeReader::Create, nullptr);
	env->AddFunction("PipeReaderForHandle", Filter::Pipe::PipeReader::CreateForHandleParams, Filter::Pipe::PipeReader::CreateForHandle, nullptr);
	env->AddFunction("PipeWriter", Filter::Pipe::PipeWriter::CreateParams, Filter::Pipe::PipeWriter::Create, nullptr);
	env->AddFunction("PipeWriterForHandle", Filter::Pipe::PipeWriter::CreateForHandleParams, Filter::Pipe::PipeWriter::CreateForHandle, nullptr);
	env->AddFunction("ssifSource", Filter::ssifSource::CreateParams, Filter::ssifSource::Create, nullptr);

	/*
	env->AddFunction("ssifSource", 
		,
		Create_SSIFSource, 0);
	env->AddFunction("mplsSource", 
		"[mpls_file]s[ssif_path]s[left_view]b[right_view]b[horizontal_stack]b[swap_views]b[intel_params]s[debug]b[use_ldecod]b",
		Create_MPLSSource, 0);
		*/
	return nullptr;
}
