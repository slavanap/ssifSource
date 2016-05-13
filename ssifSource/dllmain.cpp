#include "stdafx.h"
#include "Filter.Pipe.hpp"
#include "Filter.mplsSource.hpp"
#include "Filter.mplsSource2.hpp"
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
	env->AddFunction("mplsSource", Filter::mplsSource::CreateParams, Filter::mplsSource::Create, nullptr);
	env->AddFunction("mplsSource2", Filter::mplsSource2::CreateParams, Filter::mplsSource2::Create, nullptr);
	return nullptr;
}
