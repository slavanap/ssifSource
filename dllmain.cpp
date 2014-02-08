#include "stdafx.h"
#include "utils.h"
#include "intel_usage.h"

#define PATH_BUFFER_LENGTH 1024
string program_path;

bool DLLInit() {
	char buffer[PATH_BUFFER_LENGTH];
	int res;
	srand((unsigned int)time(NULL));
	res = GetModuleFileNameA(hInstance, buffer, PATH_BUFFER_LENGTH);
	if (res == 0) {
		MessageBoxA(HWND_DESKTOP, "Can not retrieve program path", NULL, MB_ICONERROR | MB_OK);
		return false;
	}
	program_path = buffer;
	program_path.erase(program_path.rfind("\\")+1);
	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hModule;
			if (!DLLInit())
				return FALSE;
            break;
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
	env->AddFunction("ssifSource", "[ssif_file]s[frame_count]i[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]b", Create_SSIFSource, 0);
	return 0;
}
