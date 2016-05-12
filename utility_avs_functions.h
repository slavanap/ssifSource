#pragma once
#include "avisynth.h"

#define DECLARE_AVS_FUNCTION(fname, fparams) \
	AVSValue __cdecl fname(AVSValue args, void* user_data, IScriptEnvironment* env); \
	inline void AVSRegister_ ## fname (IScriptEnvironment* env) { \
		env->AddFunction(#fname, fparams, fname, 0); \
	}

#define REGISTER_AVS_FUNCTION(fname) AVSRegister_##fname(env);

DECLARE_AVS_FUNCTION(ssCreateReadPipe, "[pipe_name]s")
DECLARE_AVS_FUNCTION(ssCreateWritePipe, "[pipe_name]s")
DECLARE_AVS_FUNCTION(ssDestroyPipe, "[pipe_cookie]i")
DECLARE_AVS_FUNCTION(ssWriteToPipe, "[pipe_name]s[clip]c")
DECLARE_AVS_FUNCTION(ssWriteToPipeHandle, "[pipe_cookie]i[clip]c")
DECLARE_AVS_FUNCTION(ssReadFromPipe, "[pipe_name]s")
DECLARE_AVS_FUNCTION(ssReadFromPipeHandle, "[pipe_cookie]i")
