#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {

	class CropDetect : public GenericVideoFilter {
	public:
		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	protected:
		void DetectForFrame(PVideoFrame vf, RECT* res);
	private:
		int m_threshold;
		CropDetect(IScriptEnvironment* env, PClip clip, const char *outfile, int threshold, int detectFrames);
	};
}