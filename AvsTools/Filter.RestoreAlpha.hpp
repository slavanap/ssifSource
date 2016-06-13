#pragma once

#include "Tools.AviSynth.Frame.hpp"

namespace Filter {

	class RestoreAlpha : public GenericVideoFilter {
	public:
		RestoreAlpha(IScriptEnvironment* env, const std::string& funcname, PClip reference);
		PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

	private:
		enum {
			SAMPLES = 3,
		};
		PClip clipOverlay[SAMPLES];
		Tools::AviSynth::Frame::Pixel bkcolor[SAMPLES];
	};

}