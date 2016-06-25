#pragma once
#include "Tools.AviSynth.Frame.hpp"

namespace Filter {

	class CropDetect : public GenericVideoFilter {
	public:
		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	protected:
		void AdjustForFrame(RECT& rect, const Tools::AviSynth::Frame& frame);
	private:
		int m_threshold;
		CropDetect(IScriptEnvironment* env, PClip clip, const char *outfile, int threshold, int detectFrames);
		bool CheckThreshold(int lum, int scale) const {
			return lum / (scale * 1000) >= m_threshold;
		}
	};

}
