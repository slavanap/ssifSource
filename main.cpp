#include "stdafx.h"
#include "utils.h"
#include "graph_creation.h"
#include "several_files.h"

// Avisynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("ssifSource2", "[ssif_file]s[frame_count]i[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]i[create_index]b", SSIFSource::Create, 0);
	env->AddFunction("ssifSource3", "[filelist]s[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]i[create_index]b", SSIFSourceExt::Create, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
