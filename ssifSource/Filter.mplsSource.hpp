#pragma once
#include "Tools.AviSynth.hpp"

namespace Filter {

	class mplsSource : public Tools::AviSynth::SourceFilterStub {
	public:
		mplsSource(IScriptEnvironment* env, AVSValue args);
		virtual ~mplsSource();

		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

		static LPCSTR CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

	private:
		int currentIndex;
		std::vector<std::string> fileNames;
		std::vector<int> frameOffsets;
		PClip currentClip;
		std::string ssifPath;
		bool flagMVC;
		bool flagSwapViews;

		AVSValue pluginParams;
		AVSValue *pluginParamValues;

		void ChangeCurrentFile(int new_idx, IScriptEnvironment* env);
	};


}
