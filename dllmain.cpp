#include "stdafx.h"
#include "utils.h"
#include "ldecod_usage.h"
#include "coreavc_usage.h"
#include "several_files.h"

// Avisynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("ssifSource", "[ssif_file]s[width]i[height]i[frame_count]i[left_track]i[right_track]i[left_264]s[right_264]s[show_params]i", Create_SSIFSource, 0);
	env->AddFunction("ssifSource2", "[ssif_file]s[frame_count]i[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]i[create_index]b", SSIFSource2::Create, 0);
	env->AddFunction("ssifSource3", "[filelist]s[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]i[create_index]b", SSIFSourceExt::Create, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hModule;
			if (!SSIFSource::DLLInit())
				return FALSE;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
