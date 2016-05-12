#include "stdafx.h"
#include "utils.h"
#include "intel_usage.h"
#include "mpls_source.h"
#include "utility_avs_functions.h"

#define PATH_BUFFER_LENGTH 8192

bool DLLInit() {
	srand((unsigned int)time(NULL));

	wchar_t *exe_filename = new wchar_t[PATH_BUFFER_LENGTH];
	if (!GetModuleFileNameW(hInstance, exe_filename, PATH_BUFFER_LENGTH)) {
		MessageBoxA(HWND_DESKTOP, "Can not retrieve program path", NULL, MB_ICONERROR | MB_OK);
		delete[] exe_filename;
		return false;
	}
	size_t len;
	StringCchLengthW(exe_filename, PATH_BUFFER_LENGTH, &len);
	while (len > 0 && exe_filename[len-1] != '\\') --len;
	if (len > 0) {
		exe_filename[len-1] = ';';
		size_t path_len = GetEnvironmentVariableW(L"PATH", exe_filename + len, PATH_BUFFER_LENGTH - len);
		if (path_len <= PATH_BUFFER_LENGTH - len) {
			SetEnvironmentVariableW(L"PATH", exe_filename);
		}
	}
	delete exe_filename;
	return true;
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
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
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

// AviSynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("ssifSource", 
		"[ssif_file]s[frame_count]i[avc_view]b[mvc_view]b[horizontal_stack]b[swap_views]b[intel_params]s[debug]b[use_ldecod]b"
		"[avc264]s[mvc264]s[muxed264]s[width]i[height]i[stop_after]i",
		Create_SSIFSource, 0);
	env->AddFunction("mplsSource", 
		"[mpls_file]s[ssif_path]s[left_view]b[right_view]b[horizontal_stack]b[swap_views]b[intel_params]s[debug]b[use_ldecod]b",
		Create_MPLSSource, 0);

	REGISTER_AVS_FUNCTION(ssCreateReadPipe)
	REGISTER_AVS_FUNCTION(ssCreateWritePipe)
	REGISTER_AVS_FUNCTION(ssDestroyPipe)
	REGISTER_AVS_FUNCTION(ssWriteToPipe)
	REGISTER_AVS_FUNCTION(ssWriteToPipeHandle)
	REGISTER_AVS_FUNCTION(ssReadFromPipe)
	REGISTER_AVS_FUNCTION(ssReadFromPipeHandle)
	return 0;
}
