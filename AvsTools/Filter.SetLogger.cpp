#include "stdafx.h"
#include <avisynth/avisynth.h>
#include <io.h>

namespace Filter {

	static int orig_stdout = _dup(1), orig_stderr = _dup(2);

	LPCSTR SetLoggerParams = "[filename]s[console]b";

	AVSValue __cdecl SetLogger(AVSValue args, void* user_data, IScriptEnvironment* env) {
		bool console = args[1].AsBool(false);
		if (console)
			AllocConsole();
		const char *filename = args[0].AsString("");
		if (!filename || !*filename) {
			if (console) {
				filename = "CONOUT$";
			}
			else {
				_dup2(orig_stdout, 1);
				_dup2(orig_stderr, 2);
				return true;
			}
		}
#pragma warning(push)
#pragma warning(disable: 4996)
		freopen(filename, "wb", stdout);
		freopen(filename, "wb", stderr);
#pragma warning(pop)
		return true;
	}

}
