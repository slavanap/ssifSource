#pragma once

namespace Filter {

	class SequentialToSeekable : public ::Tools::AviSynth::SourceFilterStub {
	public:
		SequentialToSeekable(IScriptEnvironment* env, const char* command);
		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);
		
		static LPCSTR CreateParams;
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
