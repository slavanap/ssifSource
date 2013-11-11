#include "stdafx.h"
#include "utils.h"
#include "coreavc_usage.h"

#define SSIFSOURCE2_PLUGIN "ssifSource2"
#define FORMAT_PRINTMESSAGE(x) SSIFSOURCE2_PLUGIN ": " x "\n"
#define FRAME_WAITTIMEOUT 60000 // 60 seconds
#define FRAME_SEEKBACKFRAMECOUNT 200 // 200 frames

EXTERN_C const CLSID CLSID_NullRenderer;

typedef enum {
	SN_MPEG,
	SN_MATROSKA,
	SN_MPEG4
} SplitterName;

LPOLESTR lib_MpegSplitter = T2OLE(L"MPEGSplitter.dll");
LPOLESTR lib_MatroskaSplitter = T2OLE(L"MatroskaSplitter.dll");
LPOLESTR lib_Mpeg4Splitter = T2OLE(L"MP4Splitter.dll");
LPOLESTR lib_Decoder = T2OLE(L"CoreAVCDecoder.dll");

HRESULT CreateGraph(const WCHAR* sFileName, IBaseFilter* avc_grabber, IBaseFilter* mvc_grabber, IGraphBuilder** ppGraph, SplitterName sn) {
    HRESULT hr = S_OK;
    IGraphBuilder *pGraph = NULL;
    CComQIPtr<IBaseFilter> pDecoder;
    CComQIPtr<IBaseFilter> pSplitter;
    CComQIPtr<IPropertyBag> prog_bag;
    CComQIPtr<IBaseFilter> pRenderer1, pRenderer2;
	LPOLESTR lib_Splitter;
	const CLSID *clsid_Splitter;

	switch (sn) {
		case SN_MPEG:
			lib_Splitter = lib_MpegSplitter;
			clsid_Splitter = &CLSID_MpegSplitter;
			break;
		case SN_MATROSKA:
			lib_Splitter = lib_MatroskaSplitter;
			clsid_Splitter = &CLSID_MatroskaSplitter;
			break;
		case SN_MPEG4:
			lib_Splitter = lib_Mpeg4Splitter;
			clsid_Splitter = &CLSID_Mpeg4Splitter;
			break;
		default:
			return E_UNEXPECTED;
	}

    WCHAR fullname_splitter[MAX_PATH+32];
    WCHAR fullname_decoder[MAX_PATH+32];
    size_t len;
    if (GetModuleFileNameW(hInstance, fullname_splitter, MAX_PATH) == 0)
        return E_UNEXPECTED;
    StringCchLengthW(fullname_splitter, MAX_PATH, &len);
    while (len > 0 && fullname_splitter[len-1] != '\\') --len;
    fullname_splitter[len] = '\0';
    StringCchCopyW(fullname_decoder, MAX_PATH, fullname_splitter);
    StringCchCatW(fullname_splitter, MAX_PATH+32, lib_Splitter);
    StringCchCatW(fullname_decoder, MAX_PATH+32, lib_Decoder);

    // Create the Filter Graph Manager.
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
    if (FAILED(hr)) goto lerror;
    hr = DSHelpCreateInstance(fullname_splitter, *clsid_Splitter, NULL, IID_IBaseFilter, (void**)&pSplitter);
    if (FAILED(hr)) goto lerror;
    hr = DSHelpCreateInstance(fullname_decoder, CLSID_CoreAVC, NULL, IID_IBaseFilter, (void**)&pDecoder);
    if (FAILED(hr)) goto lerror;

    LPOLESTR settings[] = {L"app_mode=1", L"ilevels=2", L"olevels=2", L"ics=5", L"di=9", L"deblock=10", L"ai=1", L"crop1088=1",
        L"vmr_ar=0", L"low_latency=0", L"brightness=0", L"contrast=0", L"saturation=0", L"use_tray=0", L"use_cuda=0"};

    prog_bag = pDecoder;
    for(int i=0; i < sizeof(settings)/sizeof(WCHAR*); ++i) {
        hr = prog_bag->Write(T2OLE(L"Settings"), &CComVariant(settings[i]));
        if (FAILED(hr)) goto lerror;
    }

    hr = pGraph->AddFilter(pDecoder, L"VDecoder");
    if (FAILED(hr)) goto lerror;
    hr = pGraph->AddFilter(pSplitter, L"VSplitter");
    if (FAILED(hr)) goto lerror;

    hr = CComQIPtr<IFileSourceFilter>(pSplitter)->Load(sFileName, NULL);
    if (FAILED(hr)) goto lerror;

    hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 0, true), GetInPin(pDecoder, 0), NULL);
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 1, true), GetInPin(pDecoder, 1), NULL);
    // Ignore the error here. For 'MVC (Full)' support

    if (avc_grabber != NULL) {
        hr = pGraph->AddFilter(avc_grabber, L"LeftView");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(pDecoder, 0), GetInPin(avc_grabber, 0), NULL);
        if (FAILED(hr)) goto lerror;
    }
    else {
        avc_grabber = pDecoder;
    }

    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer1);
    if (FAILED(hr)) goto lerror;
    hr = pGraph->AddFilter(pRenderer1, L"Renderer1");
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(avc_grabber, 0), GetInPin(pRenderer1, 0), NULL);
    if (FAILED(hr)) goto lerror;

    if (mvc_grabber != NULL) {
        hr = pGraph->AddFilter(mvc_grabber, L"RightView");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(pDecoder, 1), GetInPin(mvc_grabber, 0), NULL);
        if (FAILED(hr)) goto lerror;

        hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer2);
        if (FAILED(hr)) goto lerror;
        hr = pGraph->AddFilter(pRenderer2, L"Renderer2");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(mvc_grabber, 0), GetInPin(pRenderer2, 0), NULL);
        if (FAILED(hr)) goto lerror;
    }

    *ppGraph = pGraph;
    return S_OK;
lerror:
    prog_bag = (IUnknown*)NULL;
    pDecoder = (IUnknown*)NULL;
    pSplitter = (IUnknown*)NULL;
    if (pGraph!= NULL)
        pGraph->Release();
    return E_FAIL;
}

// === SSIFSource class =======================================================

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal) {
    const char* arg_names[2] = {NULL, NULL};
    AVSValue args[2] = {a, b};
    return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

SSIFSource2::SSIFSource2(AVSValue& args, IScriptEnvironment* env) {
    main_grabber = sub_grabber = skip_grabber = NULL;
    current_frame_number = -1;
    pGraph = NULL;
    hWindow = NULL;
    tmDuration = 0;

    params = 0;
    if (args[2].AsBool(false))
        params |= SP2_AVCVIEW;
    if (args[3].AsBool(true))
        params |= SP2_MVCVIEW;
    if (args[4].AsBool(false))
        params |= SP2_HORIZONTALSTACK;
    if (!(params & (SP2_AVCVIEW | SP2_MVCVIEW)))
        Throw("No view selected!");

    HRESULT hr = S_OK;
    CoInitialize(NULL);
    {
        USES_CONVERSION;
        CSampleGrabber *avc_grabber = NULL, *mvc_grabber = NULL;
		avc_grabber = new CSampleGrabber(&hr);
		if (params & SP2_MVCVIEW) {
			mvc_grabber = new CSampleGrabber(&hr);
			if (params & SP2_AVCVIEW) {
				main_grabber = avc_grabber;
				sub_grabber = mvc_grabber;
			}
			else {
				skip_grabber = avc_grabber;
				skip_grabber->SetEnabled(false);
				main_grabber = mvc_grabber;
			}
		}
		else {
			main_grabber = avc_grabber;
		}
		string filename = args[0].AsString();
		SplitterName sn = SN_MPEG;
		int pos = filename.find_last_of('.');
		if (pos != string::npos) {
			string ext = filename.substr(pos);
			if (!_stricmp(ext.c_str(), ".mkv")) {
				sn = SN_MATROSKA;
			}
			else if(!_stricmp(ext.c_str(), ".mp4")) {
				sn = SN_MPEG4;
			}
		}
        hr = CreateGraph(A2W(filename.c_str()), static_cast<IBaseFilter*>(avc_grabber), static_cast<IBaseFilter*>(mvc_grabber), &pGraph, sn);
        if (FAILED(hr)) {
            Clear();
            CoUninitialize();
            throw "Can't create the graph";
        }
    }
    pEvent = pGraph;
    if (!pEvent) Throw("Can't retrieve IMediaEventEx interface");
    pControl = pGraph;
    if (!pControl) Throw("Can't retrieve IMediaControl interface");
	pSeeking = pGraph;
	if (!pSeeking) Throw("Can't retrieve IMediaSeeking interface");

    // Run the graph
    hr = pControl->Run();
    if (FAILED(hr)) Throw("Can't run the graph");

    if (main_grabber != NULL && sub_grabber != NULL)
        if (main_grabber->m_Width != sub_grabber->m_Width || main_grabber->m_Height != sub_grabber->m_Height)
            Throw("Differences in output video");

    // copy VideoInfo
    frame_vi = main_grabber->avisynth_vi;
    // Get total number of frames
	printf("\n");
    LONGLONG nTotalFrames;
    pSeeking->GetDuration(&tmDuration);
	if (FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME))) {
		nTotalFrames = (tmDuration + main_grabber->m_AvgTimePerFrame/2) / main_grabber->m_AvgTimePerFrame;
		int frames = args[1].AsInt(0);
		if (frames <= 0)
			frames = FrameCountDetect(args[0].AsString(), env);
		if (frames <= 0) {
			printf(FORMAT_PRINTMESSAGE("framecount directshow value is %d"), (int)nTotalFrames);
		}
		else {
			nTotalFrames = frames;
			printf(FORMAT_PRINTMESSAGE("frame count has been forced to %d"), frames);
		}
		frame_vi.num_frames = (int)nTotalFrames;
	}
	else {
		pSeeking->GetDuration(&nTotalFrames);
		frame_vi.num_frames = (int)nTotalFrames;
	}

    vi = frame_vi;
    if (sub_grabber)
        ((params & SP2_HORIZONTALSTACK) ? vi.width : vi.height) *= 2;

	// Parsing swap_views parameter
	int flag_swap_views = args[5].AsInt(0);
	if (flag_swap_views != -1) {
		if (flag_swap_views)
			params |= SP2_SWAPVIEWS;
	}
	else {
		if (SwapViewsDetect(args[0].AsString()))
			params |= SP2_SWAPVIEWS;
	}
}

void SSIFSource2::Clear() {
    if (main_grabber != NULL) 
        main_grabber->SetEnabled(false);
    if (sub_grabber != NULL) 
        sub_grabber->SetEnabled(false);
    if (pControl) {
        pControl->StopWhenReady();

        long evCode = 0;
        LONG_PTR param1 = 0, param2 = 0;
        HRESULT hr = S_OK;
		MSG msg;

        while (1) {
            bool good = false;
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                good = true;
            }
            if (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0)))
            {
                // Invoke the callback.
                if (evCode == EC_COMPLETE) {
                    CComQIPtr<IMediaControl>(pGraph)->Stop();
                }

                // Free the event data.
                hr = pEvent->FreeEventParams(evCode, param1, param2);
                if (FAILED(hr))
                    break;
                good = true;
            }
            if (!good) break;
        }
    }
    if (pEvent) {
        pEvent->SetNotifyWindow(NULL, NULL, NULL);
        pEvent = (LPUNKNOWN)NULL;
    }
    if (pControl) {
        pControl->Stop();
        pControl = (LPUNKNOWN)NULL;
    }
    if (hWindow != NULL) {
        DestroyWindow(hWindow);
        hWindow = NULL;
    }
    if (pGraph != NULL) {
        pGraph->Release();
        pGraph = NULL;
        CoUninitialize();
    }
}

SSIFSource2::~SSIFSource2() {
    Clear();    
}

bool SSIFSource2::SwapViewsDetect(const string& filename) {
	printf(FORMAT_PRINTMESSAGE("swapviews autodetect mode on. Searching for swapviews flag in mpls files..."));

	size_t pos = filename.find_last_of('\\');
	string mpls_path = filename;
	mpls_path.erase(pos+1, string::npos);
	mpls_path += (string)"..\\..\\PLAYLIST\\";
	printf(FORMAT_PRINTMESSAGE("Searching for .mpls files in directory %s"), mpls_path.c_str());

	string ssif_filename = filename.substr(pos+1, string::npos);
	pos = ssif_filename.find_last_of('.');
	if (pos != string::npos)
		ssif_filename.erase(pos, string::npos);
	ssif_filename += (string)"M2TS";

	WIN32_FIND_DATAA ffd;
	HANDLE hFind;
	int detect_swaps = 0, all_detects = 0;

	// Search for filename
	hFind = FindFirstFileA((mpls_path + "*.mpls").c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				HANDLE hFile = CreateFileA((mpls_path + ffd.cFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (hFile != INVALID_HANDLE_VALUE) {
					int filesize = GetFileSize(hFile, NULL);
					if (filesize <= 0x38) {
						printf(FORMAT_PRINTMESSAGE("warning: too small mpls file %s"), ffd.cFileName);
					}
					else {
						char *buffer = (char*)malloc(filesize);
						if (buffer) {
							DWORD temp = 0;
							ReadFile(hFile, buffer, filesize, &temp, NULL);
							if (temp == filesize) {
								int strsz = ssif_filename.size();
								for(int i=0; i<filesize-strsz; ++i) {
									if (!strncmp(ssif_filename.c_str(), buffer+i, strsz)) {
										int bSwap = (buffer[0x38] & 0x10) != 0;
										detect_swaps += bSwap;
										all_detects++;
										printf(FORMAT_PRINTMESSAGE("Found file mention in %s file. Views swap value for this file is %s"), 
											ffd.cFileName, bSwap ? "TRUE" : "FALSE");
									}
								}
							}
							free(buffer);
						}
					}
					CloseHandle(hFile);
				}
			}
		} while (FindNextFileA(hFind, &ffd) != 0);
		FindClose(hFind);
	}
	bool res = detect_swaps > (all_detects / 2 + 1);
	printf(FORMAT_PRINTMESSAGE("Views swap guess result is %s (%d of %d)."), res ? "TRUE" : "FALSE", detect_swaps, all_detects);
	return res;;
}

int SSIFSource2::FrameCountDetect(const string& filename, IScriptEnvironment* env) {
	size_t pos = filename.find_last_of('\\');
	string path = filename;
	path.erase(pos+1, string::npos);
	string ssif_filename = filename.substr(pos+1, string::npos);
	pos = ssif_filename.find_last_of('.');
	if (pos != string::npos)
		ssif_filename.erase(pos, string::npos);
	ssif_filename = path + "..\\" + ssif_filename + (string)".M2TS";

	printf(FORMAT_PRINTMESSAGE("framecount autodetect mode on. looking for '%s' file..."), ssif_filename.c_str());

	if (env->FunctionExists("DSS2")) {
		int frames = 0;
		try	{
			const char* arg_names[1] = {NULL};
			AVSValue args[1] = {ssif_filename.c_str()};
			PClip c = (env->Invoke("DSS2", AVSValue(args, sizeof(args) / sizeof(AVSValue)), arg_names)).AsClip();
			frames = c->GetVideoInfo().num_frames;
			c = NULL;
		}
		catch (AvisynthError err) {
			printf(FORMAT_PRINTMESSAGE("Avisynth error violated during opening file with DSS2: %s"), err.msg);
		}
		return frames;
	}
	else {
		printf(FORMAT_PRINTMESSAGE("DSS2 function does not exists. Please add DSS2 plugin (avss.dll) to Avisynth plugins to make this feature work."));
		return 0;
	}
}

AVSValue __cdecl SSIFSource2::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    SSIFSource2 *obj = NULL;
    try {
        obj = new SSIFSource2(args, env);
    } catch(const char* str) {
        env->ThrowError("%s", str);
    }
    return obj;
}

void SSIFSource2::DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf, IScriptEnvironment* env, bool bSignal) {
    if (bSignal)
        SetEvent(grabber->hDataReady);
	HANDLE handles[2] = {grabber->hDataParsed, grabber->hEventDisabled};
	DWORD wait_res = WaitForMultipleObjects(2, handles, FALSE, FRAME_WAITTIMEOUT);
    if (wait_res != WAIT_OBJECT_0) {
		printf("\n" FORMAT_PRINTMESSAGE("%s Frame #%6d duplicate added (debug: g%08x m%08x s%08x)"), 
			(wait_res == WAIT_TIMEOUT) ? "Decoding frame timeout reached!!!" : 
			(wait_res == WAIT_OBJECT_0 + 1) ? "End of graph." : "Seek out of graph.",
			current_frame_number, grabber, main_grabber, sub_grabber);
        main_grabber->SetEnabled(false);
		if (sub_grabber) sub_grabber->SetEnabled(false);
    }
    if ((void*)vf)
        memcpy(vf->GetWritePtr(), grabber->pData, grabber->m_FrameSize);
    ResetEvent(grabber->hDataParsed);
}

void SSIFSource2::DropFrame(CSampleGrabber *grabber, IScriptEnvironment* env, bool bSignal) {
    DataToFrame(grabber, PVideoFrame(), env, bSignal);
}

void SSIFSource2::ParseEvents() {
    long evCode = 0;
    LONG_PTR param1 = 0, param2 = 0;
    HRESULT hr = S_OK;
    while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
        // Invoke the callback.
        if (evCode == EC_COMPLETE) {
            main_grabber->SetEnabled(false);
            if (sub_grabber) sub_grabber->SetEnabled(false);
        }

        // Free the event data.
        hr = pEvent->FreeEventParams(evCode, param1, param2);
        if (FAILED(hr))
            break;
    }
}

void SSIFSource2::SeekToFrame(int framenumber, bool& bMainSignal, IScriptEnvironment* env) {
	printf(FORMAT_PRINTMESSAGE("seeking to frame %d (lastframe_number = %d)"), framenumber, current_frame_number);

	REFERENCE_TIME lPos = main_grabber->m_AvgTimePerFrame * framenumber;
	main_grabber->SetEnabled(false);
	if (sub_grabber) sub_grabber->SetEnabled(false);
	pControl->Pause();
	HRESULT hr = pSeeking->SetPositions(&lPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

	main_grabber->SetEnabled(true);
	if (sub_grabber) sub_grabber->SetEnabled(true);
	pControl->Run();
	current_frame_number = framenumber;
}

PVideoFrame WINAPI SSIFSource2::GetFrame(int n, IScriptEnvironment* env) {
	bool bMainSignal = true;
	bool bSubSignal = true;

    if (n != current_frame_number+1) {
        SeekToFrame(n, bMainSignal, env);
		int skip_frames = n - current_frame_number;
		while (skip_frames-- > 0) {
			DropFrame(main_grabber, env, bMainSignal);
			if (sub_grabber) DropFrame(sub_grabber, env);
			bMainSignal = true;
		}
		current_frame_number = n;
    }
	else
		++current_frame_number;

	if (main_grabber->GetEnabled())
		pControl->Run();
	// Sync running states
	if (!main_grabber->GetEnabled() && sub_grabber) 
		sub_grabber->SetEnabled(false);
	if (sub_grabber && !sub_grabber->GetEnabled()) 
		main_grabber->SetEnabled(false);

	// Dumping process
	PVideoFrame frame_main, frame_sub;
	frame_main = env->NewVideoFrame(frame_vi);
	DataToFrame(main_grabber, frame_main, env, bMainSignal);
	if (sub_grabber) {
		frame_sub = env->NewVideoFrame(frame_vi);
		DataToFrame(sub_grabber, frame_sub, env, bSubSignal);
	}

	if (sub_grabber)
		return ClipStack(env, 
			new FrameHolder(frame_vi, ((params & SP2_SWAPVIEWS) == 0) ? frame_main : frame_sub), 
			new FrameHolder(frame_vi, ((params & SP2_SWAPVIEWS) == 0) ? frame_sub : frame_main), 
			(params & SP2_HORIZONTALSTACK) != 0) -> GetFrame(0, env);
	else
		return frame_main;
}
