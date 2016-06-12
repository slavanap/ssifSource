#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {

	class SequentialToSeekable : public Tools::AviSynth::SourceFilterStub {
	public:
		SequentialToSeekable(IScriptEnvironment* env, const char* command);
		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);
		
		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	private:
		IScriptEnvironment *env;
		std::string command;
		PClip clip;
		PVideoFrame frame;
		int last_index;

		PClip eval(const char* command);
	};

}
