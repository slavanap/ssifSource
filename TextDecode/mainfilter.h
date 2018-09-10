#pragma once
#include "Tools.AviSynth.Frame.hpp"

class TextDecode : public GenericVideoFilter {
public:
	TextDecode(IScriptEnvironment* env, PClip input, PClip symbols, int detect_offset, int trusted_lines);
	~TextDecode();
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

	static AvsParams CreateParams;
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
	Tools::AviSynth::Frame _symbols;
	std::vector<std::string> _lines;
	std::ofstream _file;
	int _detect_offset;
	int _mismatch_frames;
	int _prev_output_frame;
	int _trusted_lines;
};
