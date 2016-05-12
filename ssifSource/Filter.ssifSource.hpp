#pragma once
#include "Tools.AviSynth.hpp"
#include "Tools.Pipe.hpp"
#include "Tools.WinApi.hpp"

namespace Filter {

	using namespace Tools::Pipe;
	using namespace Tools::WinApi;

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
		std::unique_ptr<ProcessHolder> processMuxer, processDecoder;
		std::unique_ptr<FrameSeparator> frLeft, frRight;
		std::unique_ptr<Thread> proxyThread1, proxyThread2, proxyThread3;
		bool pipesOverWarning;
		CComPtr<IGraphBuilder> pGraph;
		CComPtr<IBaseFilter> pSplitter;
		UniqueIdHolder uniqueId;
		PVideoFrame vfLeft, vfRight;

		void InitVariables();
		void InitDemuxer();
		void InitMuxer();
		void InitDecoder();
		void InitComplete();
		void DeinitAll();
		PVideoFrame ReadFrame(IScriptEnvironment* env, const std::unique_ptr<FrameSeparator>& frSep);
		void DropFrame(const std::unique_ptr<FrameSeparator>& frSep);

		static HRESULT CreateGraph(LPCWSTR fnSource, LPCWSTR fnBase, LPCWSTR fnDept,
			CComPtr<IGraphBuilder>& poGraph, CComPtr<IBaseFilter>& poSplitter);
		void ParseEvents();
	};

}