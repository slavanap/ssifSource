#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {

	extern AvsParams SetLoggerParams;
	AVSValue SetLogger(AVSValue args, void* user_data, IScriptEnvironment* env);

}
