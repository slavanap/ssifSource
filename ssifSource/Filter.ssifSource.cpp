#include "stdafx.h"
#include "Filter.ssifSource.hpp"
#include "Tools.DirectShow.hpp"
#include "Tools.WinApi.hpp"
#include "DumpFilter.hpp"

using namespace Tools::AviSynth;
using namespace Tools::DirectShow;
using namespace Tools::WinApi;

#define FILTER_NAME "ssifSource"

namespace Filter {

	// paths for binaries for debugging
#ifdef _DEBUG
	std::string BinPath = "..\\..\\bin\\";
#else
	std::string BinPath;
#endif

	enum FrameSpecial {
		FRAME_START = -1,
		FRAME_BLACK = -2,
	};

	ssifSource::ssifSource(const Params& creationParams) :
		SourceFilterStub(VideoInfo())	// vi will be initialized in InitComplete function
	{
		params = creationParams;
		try {
			InitVariables();
			InitDemuxer();
			if (params.stopAfter != SA_DEMUXER) {
				if (!params.left264Filename.empty() && !params.right264Filename.empty())
					InitMuxer();
				if (params.stopAfter != SA_MUXER) {
					if (!params.h264MuxedFilename.empty())
						InitDecoder();
					else
						throw std::runtime_error("Can't decode nothing");
				}
			}
			InitComplete();
		}
		catch (...) {
			DeinitAll();
			throw;
		}
	}

	ssifSource::~ssifSource() {
		DeinitAll();
	}

	void ssifSource::InitVariables() {
		// VideoType
		frame_vi.width = params.width;
		frame_vi.height = params.height;
		frame_vi.fps_numerator = 24000;
		frame_vi.fps_denominator = 1001;
		frame_vi.pixel_type = VideoInfo::CS_I420;
		frame_vi.num_frames = params.frameCount;
		frame_vi.audio_samples_per_second = 0;

		lastFrame = FRAME_BLACK;
		
		if (params.flagDebug) {
			AllocConsole();
#pragma warning(push)
#pragma warning(disable:4996)
			freopen("CONOUT$", "wb", stdout);
#pragma warning(pop)
		}

		pipesOverWarning = false;

		// WARNING: keep in mind this swapping!
		if (TEST(params.showParam, SP_SWAPVIEWS) && !TESTALL(params.showParam, SP_LEFTVIEW | SP_RIGHTVIEW))
			params.showParam ^= SP_LEFTVIEW | SP_RIGHTVIEW;
	}

	void ssifSource::InitDemuxer() {
		if (params.ssifFilename.empty())
			return;

		USES_CONVERSION;
		std::string
			fiBase = uniqueId.MakePipeName("base.h264"),
			fiDept = uniqueId.MakePipeName("dept.h264"),
			foBase = uniqueId.MakePipeName("base_merge.h264"),
			foDept = uniqueId.MakePipeName("dept_merge.h264");

		if (params.stopAfter == SA_DEMUXER) {
			fiBase = params.left264Filename;
			fiDept = params.right264Filename;
		}
		else {
			if (params.left264Filename.empty())
				proxyThread2 = std::make_unique<ProxyThread>(fiBase.c_str(), foBase.c_str());
			else
				proxyThread2 = std::make_unique<CloneThread>(fiBase.c_str(), foBase.c_str(), params.left264Filename.c_str());
			params.left264Filename = foBase;

			if (TEST(params.showParam, SP_RIGHTVIEW)) {
				if (params.right264Filename.empty())
					proxyThread3 = std::make_unique<ProxyThread>(fiDept.c_str(), foDept.c_str());
				else
					proxyThread3 = std::make_unique<CloneThread>(fiDept.c_str(), foDept.c_str(), params.right264Filename.c_str());
			}
			params.right264Filename = foDept;
		}

		CoInitialize(NULL);
		HRESULT res = CreateGraph(
			A2W(params.ssifFilename.c_str()),
			A2W(fiBase.c_str()),
			TEST(params.showParam, SP_RIGHTVIEW) ? A2W(fiDept.c_str()) : NULL,
			pGraph, pSplitter);
		if (FAILED(res))
			throw std::runtime_error(format("Can't create graph. Error: %s (0x%08x)", 1024, GetErrorMessage(res).c_str(), res));

		res = CComQIPtr<IMediaControl>(pGraph)->Run();
		if (FAILED(res))
			throw std::runtime_error(format("Can't start the graph. Error: %s (0x%08x)", 1024, GetErrorMessage(res).c_str(), res));
		ParseEvents();

		// Retrieve width & height
		CComPtr<IPin> pPin = GetOutPin(pSplitter, 0);
		CComPtr<IEnumMediaTypes> pEnum;
		AM_MEDIA_TYPE *pmt = nullptr;
		res = pPin->EnumMediaTypes((IEnumMediaTypes**)&pEnum);
		if (SUCCEEDED(res)) {
			while (res = pEnum->Next(1, &pmt, nullptr), res == S_OK) {
				if (pmt->majortype == MEDIATYPE_Video) {
					if (pmt->formattype == FORMAT_MPEG2_VIDEO) {
						// Found a match
						int *vih = (int*)pmt->pbFormat;
						frame_vi.width = params.width = vih[19];
						frame_vi.height = params.height = vih[20];
						DeleteMediaType(pmt);
						break;
					}
				}
				DeleteMediaType(pmt);
			}
			pEnum = nullptr;
			pPin = nullptr;
		}
	}

	void ssifSource::InitMuxer() {
		if (!TEST(params.showParam, SP_RIGHTVIEW)) {
			params.h264MuxedFilename = params.left264Filename;
			return;
		}

		std::string
			fiMuxed = uniqueId.MakePipeName("muxed.h264"),
			foMuxed = uniqueId.MakePipeName("intel_input.h264");

		if (params.stopAfter == SA_MUXER) {
			fiMuxed = params.h264MuxedFilename;
		}
		else {
			if (params.h264MuxedFilename.empty())
				proxyThread1 = std::make_unique<ProxyThread>(fiMuxed.c_str(), foMuxed.c_str());
			else
				proxyThread1 = std::make_unique<CloneThread>(fiMuxed.c_str(), foMuxed.c_str(), params.h264MuxedFilename.c_str());
			params.h264MuxedFilename = foMuxed;
		}

		std::string
			name_muxer = BinPath + "mvccombine.exe",
			cmd_muxer = format("\"%s\" -l \"%s\" -r \"%s\" -o \"%s\" ", 1024,
				name_muxer.c_str(), params.left264Filename.c_str(), params.right264Filename.c_str(), fiMuxed.c_str());
		processMuxer = std::make_unique<ProcessHolder>(name_muxer, cmd_muxer, params.flagDebug);
	}

	void ssifSource::InitDecoder() {
		bool flag_mvc = TEST(params.showParam, SP_RIGHTVIEW);
		std::string name_decoder, s_dec_left_write, s_dec_right_write, s_dec_out, cmd_decoder;

		if (params.flagUseLdecod) {
			name_decoder = BinPath + "ldecod.exe";
			s_dec_left_write = uniqueId.MakePipeName("1_ViewId0000.yuv");
			s_dec_right_write = uniqueId.MakePipeName("1_ViewId0001.yuv");
			s_dec_out = uniqueId.MakePipeName("1.yuv");

			cmd_decoder = "\"" + name_decoder + "\" -p InputFile=\"" + params.h264MuxedFilename + "\" "
				"-p OutputFile=\"" + s_dec_out + "\" "
				"-p WriteUV=1 -p FileFormat=0 -p RefOffset=0 -p POCScale=2"
				"-p DisplayDecParams=1 -p ConcealMode=0 -p RefPOCGap=2 -p POCGap=2 -p Silent=0 -p IntraProfileDeblocking=1 -p DecFrmNum=0 "
				"-p DecodeAllLayers=1";
			flag_mvc = true;
		}
		else {
			name_decoder = BinPath + "sample_decode.exe",
				s_dec_left_write = uniqueId.MakePipeName("output_0.yuv"),
				s_dec_right_write = uniqueId.MakePipeName("output_1.yuv"),
				s_dec_out = uniqueId.MakePipeName("output"),
				cmd_decoder = "\"" + name_decoder + "\" ";
			if (!(!params.intelDecoderParam.empty() && params.intelDecoderParam[0] != '-'))
				cmd_decoder += (flag_mvc ? "mvc" : "h264");
			cmd_decoder += " " + params.intelDecoderParam + " -i \"" + params.h264MuxedFilename + "\" -o " + s_dec_out;
		}

		int framesize = frame_vi.width*frame_vi.height + (frame_vi.width*frame_vi.height / 2)&~1;
		if (!flag_mvc)
			s_dec_left_write = s_dec_out;

		frLeft = std::make_unique<FrameSeparator>(s_dec_left_write.c_str(), framesize);
		if (flag_mvc)
			frRight = std::make_unique<FrameSeparator>(s_dec_right_write.c_str(), framesize);

		processDecoder = std::make_unique<ProcessHolder>(name_decoder, cmd_decoder, params.flagDebug);
	}

	void ssifSource::InitComplete() {
		vi = frame_vi;
		if (TESTALL(params.showParam, SP_LEFTVIEW | SP_RIGHTVIEW))
			(TEST(params.showParam, SP_HORIZONTAL) ? vi.width : vi.height) *= 2;

		processMuxer->Resume();
		processDecoder->Resume();
		if (params.stopAfter == SA_DECODER)
			lastFrame = FRAME_START;
	}

	void ssifSource::DeinitAll() {
		processDecoder.reset();
		processMuxer.reset();
		proxyThread1.reset();
		proxyThread2.reset();
		proxyThread3.reset();
		frLeft.reset();
		frRight.reset();
		pSplitter = nullptr;
		if (pGraph != nullptr) {
			// very dirty HACK!!!
			static_cast<IUnknown*>(pGraph)->AddRef();
			while (static_cast<IUnknown*>(pGraph)->Release() > 1);
		}
		pGraph = nullptr;
		CoUninitialize();
	}

	HRESULT ssifSource::CreateGraph(LPCWSTR fnSource, LPCWSTR fnBase, LPCWSTR fnDept,
		CComPtr<IGraphBuilder>& poGraph, CComPtr<IBaseFilter>& poSplitter)
	{
		USES_CONVERSION;
		std::wstring BinPathW(A2W(BinPath.c_str()));
		BinPathW += L"MpegSplitter_mod.ax";

		HRESULT hr = S_OK;
		IGraphBuilder *pGraph = nullptr;
		CComQIPtr<IBaseFilter> pSplitter;
		CComQIPtr<IBaseFilter> pDumper1, pDumper2;
		LPOLESTR lib_Splitter = T2OLE(&BinPathW[0]);
		const CLSID *clsid_Splitter = &CLSID_MpegSplitter;

		WCHAR fullname_splitter[MAX_PATH + 64];
		size_t len;
		if (GetModuleFileNameW(hInstance, fullname_splitter, MAX_PATH) == 0)
			return E_UNEXPECTED;
		StringCchLengthW(fullname_splitter, MAX_PATH, &len);
		while (len > 0 && fullname_splitter[len - 1] != '\\') --len;
		fullname_splitter[len] = '\0';
		StringCchCatW(fullname_splitter, MAX_PATH + 64, lib_Splitter);

		// Create the Filter Graph Manager.
		printf("ssifSource4: Create graph items ... ");
		hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
		if (FAILED(hr)) goto lerror;
		hr = DSHelpCreateInstance(fullname_splitter, *clsid_Splitter, nullptr, IID_IBaseFilter, (void**)&pSplitter);
		if (FAILED(hr)) goto lerror;
		hr = CreateDumpFilter(IID_IBaseFilter, (void**)&pDumper1);
		if (FAILED(hr)) goto lerror;
		if (fnDept) {
			hr = CreateDumpFilter(IID_IBaseFilter, (void**)&pDumper2);
			if (FAILED(hr)) goto lerror;
		}
		printf("OK\n");

		printf("ssifSource4: Load source ... ");
		hr = pGraph->AddFilter(pSplitter, L"VSplitter");
		if (FAILED(hr)) goto lerror;
		hr = CComQIPtr<IFileSourceFilter>(pSplitter)->Load(fnSource, nullptr);
		if (FAILED(hr)) goto lerror;
		printf("OK\n");

		printf("ssifSource4: Add Dumpers ... ");
		hr = pGraph->AddFilter(pDumper1, L"VDumper1");
		if (FAILED(hr)) goto lerror;
		hr = CComQIPtr<IFileSinkFilter>(pDumper1)->SetFileName(fnBase, nullptr);
		if (FAILED(hr)) goto lerror;

		if (pDumper2) {
			hr = pGraph->AddFilter(pDumper2, L"VDumper2");
			if (FAILED(hr)) goto lerror;
			hr = CComQIPtr<IFileSinkFilter>(pDumper2)->SetFileName(fnDept, nullptr);
			if (FAILED(hr)) goto lerror;
		}
		printf("OK\n");

		printf("ssifSource4: Graph connection ... ");
		hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 0, true), GetInPin(pDumper1, 0), nullptr);
		if (FAILED(hr)) goto lerror;
		if (pDumper2) {
			hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 1, true), GetInPin(pDumper2, 0), nullptr);
			if (FAILED(hr)) goto lerror;
		}
		printf("OK\n");

		poGraph = pGraph;
		poSplitter = pSplitter;
		return S_OK;
	lerror:
		return hr;
	}

	void ssifSource::ParseEvents() {
		if (!pGraph)
			return;

		CComQIPtr<IMediaEventEx> pEvent = pGraph;
		long evCode = 0;
		LONG_PTR param1 = 0, param2 = 0;
		HRESULT hr = S_OK;
		while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
			// Invoke the callback.
			if (evCode == EC_COMPLETE) {
				CComQIPtr<IMediaControl>(pGraph)->Stop();
				pSplitter = nullptr;
			}

			// Free the event data.
			hr = pEvent->FreeEventParams(evCode, param1, param2);
			if (FAILED(hr))
				break;
		}
	}

	PVideoFrame ssifSource::ReadFrame(IScriptEnvironment* env, const std::unique_ptr<FrameSeparator>& frSep) {
		PVideoFrame res = env->NewVideoFrame(frame_vi);
		if (!frSep) {
			if (!pipesOverWarning) {
				fprintf(stderr, "\n" FILTER_NAME ": Warning! Decoder output finished. Frame separator can't read next frames. "
					"Last frame will be duplicated as long as necessary - %d times.\n", vi.num_frames - lastFrame);
				pipesOverWarning = true;
			}
		}
		else
			frSep->WaitForData();
		memcpy(res->GetWritePtr(), (char*)frSep->buffer, frSep->chunkSize);
		frSep->DataParsed();
		return res;
	}

	void ssifSource::DropFrame(const std::unique_ptr<FrameSeparator>& frSep) {
		if (frSep != nullptr) {
			if (frSep)
				frSep->WaitForData();
			frSep->DataParsed();
		}
	}

	PVideoFrame WINAPI ssifSource::GetFrame(int n, IScriptEnvironment* env) {
		ParseEvents();

		if (lastFrame == n) {
			// nothing to do.
		}
		else if (lastFrame + 1 != n || n >= vi.num_frames) {
			std::stringstream errstream;
			if ((lastFrame == FRAME_BLACK) && (params.stopAfter != SA_DECODER)) {
				if (pGraph != nullptr) {
					CComQIPtr<IMediaSeeking> pSeeking = pGraph;
					REFERENCE_TIME tmDuration, tmPosition;
					pSeeking->GetDuration(&tmDuration);
					pSeeking->GetCurrentPosition(&tmPosition);
					int value = (int)((double)tmPosition / tmDuration * 100.0);
					errstream << "Demuxing process: " << value << "% completed.";
				}
				else {
					errstream << "Demuxing process completed.";
				}
			}
			else {
				errstream << "ERROR:\\n"
					"Can't retrieve frame #" + IntToStr(n) + " !\\n";
				if (lastFrame >= vi.num_frames - 1)
					errstream << "Video sequence is over. Reload the script.\\n";
				else
					errstream << "Frame #" << lastFrame + 1 << " should be the next frame.\\n";
				errstream << "Note: " FILTER_NAME " filter supports only sequential frame rendering.";
			}
			const char* arg_names1[2] = { 0, "color_yuv" };
			AVSValue args1[2] = { this, 0 };
			PClip resultClip1 = (env->Invoke("BlankClip", AVSValue(args1, 2), arg_names1)).AsClip();

			const char* arg_names2[6] = { 0, 0, "lsp", "size", "text_color", "halo_color" };
			AVSValue args2[6] = { resultClip1, AVSValue(errstream.str().c_str()), 10, 50, 0xFFFFFF, 0x000000 };
			PClip resultClip2 = (env->Invoke("Subtitle", AVSValue(args2, 6), arg_names2)).AsClip();
			return resultClip2->GetFrame(n, env);
		}
		else {
			lastFrame = n;
			if (TEST(params.showParam, SP_LEFTVIEW))
				vfLeft = ReadFrame(env, frLeft);
			else if (frLeft)
				DropFrame(frLeft);
			if (TEST(params.showParam, SP_RIGHTVIEW))
				vfRight = ReadFrame(env, frRight);
			else if (frRight)
				DropFrame(frRight);
		}

		if (TESTALL(params.showParam, SP_LEFTVIEW | SP_RIGHTVIEW)) {
			if (!TEST(params.showParam, SP_SWAPVIEWS))
				return FrameStack(env, frame_vi, vfLeft, vfRight, TEST(params.showParam, SP_HORIZONTAL));
			else
				return FrameStack(env, frame_vi, vfRight, vfLeft, TEST(params.showParam, SP_HORIZONTAL));
		}
		else if (TEST(params.showParam, SP_LEFTVIEW))
			return vfLeft;
		else
			return vfRight;
	}



	LPCSTR ssifSource::CreateParams =
		"[ssif_file]s"
		"[frame_count]i"
		"[avc_view]b"
		"[mvc_view]b"
		"[horizontal_stack]b"
		"[swap_views]b"
		"[intel_params]s"
		"[debug]b"
		"[use_ldecod]b"
		"[avc264]s"
		"[mvc264]s"
		"[muxed264]s"
		"[width]i"
		"[height]i"
		"[stop_after]i";

	AVSValue ssifSource::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		Params params;

		// required
		params.ssifFilename = args[0].AsString("");
		params.frameCount = args[1].AsInt();

		// common group
		params.showParam =
			(args[2].AsBool(true) ? SP_LEFTVIEW : 0) |
			(args[3].AsBool(true) ? SP_RIGHTVIEW : 0) |
			(args[4].AsBool(false) ? SP_HORIZONTAL : 0) |
			(args[5].AsBool(false) ? SP_SWAPVIEWS : 0);
		params.intelDecoderParam = args[6].AsString("-d3d");
		params.flagDebug = args[7].AsBool(false);
		if (!TEST(params.showParam, SP_LEFTVIEW | SP_RIGHTVIEW))
			env->ThrowError(FILTER_NAME ": can't show nothing");
		params.flagUseLdecod = args[8].AsBool(true);

		// dump group
		params.left264Filename = args[9].AsString("");
		params.right264Filename = args[10].AsString("");
		params.h264MuxedFilename = args[11].AsString("");
		params.width = args[12].AsInt(1920);
		params.height = args[13].AsInt(1080);
		params.stopAfter = static_cast<StopAfterEnum>(args[14].AsInt(SA_DECODER));

		try {
			return new ssifSource(params);
		}
		catch (const std::exception& ex) {
			env->ThrowError(format("ERROR in %s : %s", 1024, FILTER_NAME, ex.what()).c_str());
		}
		return AVSValue();
	}

}
