#pragma once
#include "Tools.Pipe.hpp"
#include "Tools.AviSynth.hpp"

namespace Filter {

	using namespace Tools::Pipe;
	
	class ssifSource : public Tools::AviSynth::SourceFilterStub {
	public:

		enum ShowEnum {
			SP_LEFTVIEW = 0x01,
			SP_RIGHTVIEW = 0x02,
			SP_HORIZONTAL = 0x04,
			SP_SWAPVIEWS = 0x08,
			SP_MASK = 0x0F
		};

		enum StopAfterEnum {
			SA_DEMUXER = 0,
			SA_MUXER = 1,
			SA_DECODER = 2
		};

		struct Params {
			std::string ssifFilename;
			std::string h264MuxedFilename;
			std::string left264Filename;
			std::string right264Filename;
			int width;
			int height;
			int frameCount;
			int showParam;	// type: ShowEnum
			std::string intelDecoderParam;
			bool flagDebug;
			StopAfterEnum stopAfter;
			bool flagUseLdecod;

			Params() :
				width(0),
				height(0),
				frameCount(0),
				showParam(SP_LEFTVIEW | SP_RIGHTVIEW),
				flagDebug(false),
				stopAfter(SA_DECODER),
				flagUseLdecod(true)
			{
			}
		};

		ssifSource(const Params& params);
		ssifSource(const ssifSource& obj) = delete;
		~ssifSource();

		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

		static LPCSTR CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

	private:
		Params params;
		VideoInfo frame_vi;
		int lastFrame;
		STARTUPINFOA SI;
		PROCESS_INFORMATION PI1, PI2;
		FrameSeparator *frLeft, *frRight;
		Thread *dupThread1, *dupThread2, *dupThread3;
		int unic_number;
		bool pipesOverWarning;
		CComPtr<IGraphBuilder> pGraph;
		CComPtr<IBaseFilter> pSplitter;
		HANDLE hUniqueSemaphore;
		PVideoFrame vfLeft, vfRight;

		void InitVariables();
		void InitDemuxer();
		void InitMuxer();
		void InitDecoder();
		void InitComplete();
		void DeinitAll();
		PVideoFrame ReadFrame(IScriptEnvironment* env, FrameSeparator* frSep);
		void DropFrame(FrameSeparator* frSep);

		static std::string MakePipeName(int id, const std::string& name);
		static HRESULT CreateGraph(LPCWSTR fnSource, LPCWSTR fnBase, LPCWSTR fnDept,
			CComPtr<IGraphBuilder>& poGraph, CComPtr<IBaseFilter>& poSplitter);
		void ParseEvents();
	};

}