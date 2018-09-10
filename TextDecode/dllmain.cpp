#include "stdafx.h"
#include <Tools.WinApi.hpp>

#include "mainfilter.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
			return Tools::WinApi::InitLibrary(hModule);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
	return TRUE;
}

// AviSynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("TextDecode", TextDecode::CreateParams, TextDecode::Create, nullptr);
	return nullptr;
}
