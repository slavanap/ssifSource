#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Tools.WinApi.hpp>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			return Tools::WinApi::InitLibrary(hModule);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}


#include <avisynth/avisynth.h>

extern "C" __declspec(dllexport)
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	return NULL;
}
