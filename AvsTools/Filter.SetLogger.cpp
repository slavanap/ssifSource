#include "stdafx.h"
#include "Filter.SetLogger.hpp"
#include <io.h>
#include <stdio.h>

namespace Filter {

	static int orig_stdout = _dup(1), orig_stderr = _dup(2);

	AvsParams SetLoggerParams = "[filename]s[console]b";

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
				return !_dup2(orig_stdout, 1)
					&& !_dup2(orig_stderr, 2);
			}
		}
#pragma warning(push)
#pragma warning(disable: 4996)
		return !!freopen(filename, "wb", stdout)
			&& !!freopen(filename, "wb", stderr);
#pragma warning(pop)
	}

}
