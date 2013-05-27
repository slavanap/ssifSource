#include "stdafx.h"
#include "utils.h"
#include "graph_creation.h"

#define SSIFSOURCE2_PLUGIN "ssifSource2"
#define FORMAT_PRINTMESSAGE(x) SSIFSOURCE2_PLUGIN ": " x "\n"
#define FRAME_SEEKBACKFRAMECOUNT 200 // 200 frames

EXTERN_C const CLSID CLSID_NullRenderer;

LPOLESTR lib_splitter = T2OLE(L"MPEGSplitter.dll");
LPOLESTR lib_decoder = T2OLE(L"CoreAVCDecoder.dll");

HRESULT CreateGraph(const WCHAR* sFileName, IBaseFilter* avc_grabber, IBaseFilter* mvc_grabber, IGraphBuilder** ppGraph) {
    HRESULT hr = S_OK;
    IGraphBuilder *pGraph = NULL;
    CComQIPtr<IBaseFilter> pDecoder;
    CComQIPtr<IBaseFilter> pSplitter;
    CComQIPtr<IPropertyBag> prog_bag;
    CComQIPtr<IBaseFilter> pRenderer1, pRenderer2;

    WCHAR fullname_splitter[MAX_PATH+32];
    WCHAR fullname_decoder[MAX_PATH+32];
    size_t len;
    if (GetModuleFileNameW(hInstance, fullname_splitter, MAX_PATH) == 0)
        return E_UNEXPECTED;
    StringCchLengthW(fullname_splitter, MAX_PATH, &len);
    while (len > 0 && fullname_splitter[len-1] != '\\') --len;
    fullname_splitter[len] = '\0';
    StringCchCopyW(fullname_decoder, MAX_PATH, fullname_splitter);
    StringCchCatW(fullname_splitter, MAX_PATH+32, lib_splitter);
    StringCchCatW(fullname_decoder, MAX_PATH+32, lib_decoder);

    // Create the Filter Graph Manager.
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
    if (FAILED(hr)) goto lerror;
    hr = DSHelpCreateInstance(fullname_splitter, CLSID_MPC_MPEG1Splitter, NULL, IID_IBaseFilter, (void**)&pSplitter);
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

    hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 0), GetInPin(pDecoder, 0), NULL);
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 1), GetInPin(pDecoder, 1), NULL);
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

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal = false) {
    const char* arg_names[2] = {NULL, NULL};
    AVSValue args[2] = {a, b};
    return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

SSIFSource::SSIFSource(AVSValue& args, IScriptEnvironment* env) {
    main_grabber = sub_grabber = NULL;
    current_frame_number = -1;
    pGraph = NULL;
    hWindow = NULL;
    tmDuration = 0;

    params = 0;
    if (args[2].AsBool(false))
        params |= SP_AVCVIEW;
    if (args[3].AsBool(true))
        params |= SP_MVCVIEW;
    if (args[4].AsBool(false))
        params |= SP_HORIZONTALSTACK;
    if (!(params & (SP_AVCVIEW | SP_MVCVIEW)))
        Throw("No view selected!");

    HRESULT hr = S_OK;
    CoInitialize(NULL);
    {
        USES_CONVERSION;
        CSampleGrabber *avc_grabber = NULL, *mvc_grabber = NULL;
        if (params & SP_AVCVIEW)
            main_grabber = avc_grabber = new CSampleGrabber(&hr);
        if (params & SP_MVCVIEW)
            ((main_grabber == NULL) ? main_grabber : sub_grabber) = mvc_grabber = new CSampleGrabber(&hr);
        LPCSTR filename = args[0].AsString();
        hr = CreateGraph(A2W(filename), static_cast<IBaseFilter*>(avc_grabber), static_cast<IBaseFilter*>(mvc_grabber), &pGraph);
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
//    pEvent->SetNotifyWindow((OAHWND)hWindow, WM_GRAPH_EVENT, NULL);

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
    {
        CComQIPtr<IMediaSeeking> pSeeking = pGraph;
        LONGLONG nTotalFrames;
        pSeeking->GetDuration(&tmDuration);
		if (FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME))) {
			nTotalFrames = (tmDuration + main_grabber->m_AvgTimePerFrame/2) / main_grabber->m_AvgTimePerFrame;
			if (args[1].AsInt(0) == 0) {
				printf(FORMAT_PRINTMESSAGE("framecount directshow value is %d. Trying to get it more precisely..."), (int)nTotalFrames);
				SeekToFrame((int)nTotalFrames, env);
				main_grabber->nFrame = 0;
				main_grabber->SetEnabled(true);
				if (sub_grabber) sub_grabber->SetEnabled(false);
				pControl->Run();
				int last_cn = current_frame_number;
				while(!main_grabber->bComplited) {
					DropFrame(main_grabber, env);
					current_frame_number++;
				}
				frame_vi.num_frames = main_grabber->nFrame + last_cn + 1;
				printf(FORMAT_PRINTMESSAGE("detected %d frames + %d additional frames = %d frames"), 
					last_cn+1, main_grabber->nFrame, frame_vi.num_frames);
				SeekToFrame(0, env);
			}
			else {
				frame_vi.num_frames = args[1].AsInt((int)nTotalFrames);
				printf(FORMAT_PRINTMESSAGE("frame count has been forced to %d"), frame_vi.num_frames);
			}
		}
		else {
			pSeeking->GetDuration(&nTotalFrames);
			frame_vi.num_frames = (int)nTotalFrames;
		}
    }

    vi = frame_vi;
    if (sub_grabber)
        ((params & SP_HORIZONTALSTACK) ? vi.width : vi.height) *= 2;

	// Parsing swap_views parameter
	int flag_swap_views = args[5].AsInt(-1);
	if (flag_swap_views != -1) {
		if (flag_swap_views)
			params |= SP_SWAPVIEWS;
	}
	else {
		if (SwapViewsDetect(args[0].AsString()))
			params |= SP_SWAPVIEWS;
	}
}

void SSIFSource::Clear() {
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

SSIFSource::~SSIFSource() {
    Clear();    
}

bool SSIFSource::SwapViewsDetect(const string& filename) {
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

AVSValue __cdecl SSIFSource::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    SSIFSource *obj = NULL;
    try {
        obj = new SSIFSource(args, env);
    } catch(const char* str) {
        env->ThrowError("%s", str);
    }
    return obj;
}

void SSIFSource::DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf, IScriptEnvironment* env, bool bSignal) {
    if (bSignal)
        SetEvent(grabber->hDataReady);
	HANDLE handles[2] = {grabber->hDataParsed, grabber->hEventDisabled};
    if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != WAIT_OBJECT_0) {
        printf("\n" FORMAT_PRINTMESSAGE("Seek out of graph. %6d frame duplicate added (debug: g%08x m%08x s%08x)"), 
            current_frame_number, grabber, main_grabber, sub_grabber);
        grabber->SetEnabled(false);
    }
    if ((void*)vf)
        memcpy(vf->GetWritePtr(), grabber->pData, grabber->m_FrameSize);
    ResetEvent(grabber->hDataParsed);
}

void SSIFSource::DropFrame(CSampleGrabber *grabber, IScriptEnvironment* env, bool bSignal) {
    DataToFrame(grabber, PVideoFrame(), env, bSignal);
}

void SSIFSource::ParseEvents() {
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

void SSIFSource::SeekToFrame(int framenumber, bool& bMainSignal, IScriptEnvironment* env) {
	printf(FORMAT_PRINTMESSAGE("seeking to frame %d (lastframe_number = %d)"), framenumber, current_frame_number);

	CComQIPtr<IMediaSeeking> pSeeking = pGraph;
	REFERENCE_TIME lPos = main_grabber->m_AvgTimePerFrame * framenumber;
	int skip_frames = 0;

	while (1) {
		if (lPos < 0) {
			skip_frames += (int)(lPos / main_grabber->m_AvgTimePerFrame);
			lPos = 0;
		}

		main_grabber->SetEnabled(false);
		if (sub_grabber) sub_grabber->SetEnabled(false);
		pControl->Pause();

		HRESULT hr = pSeeking->SetPositions(&lPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

		main_grabber->SetEnabled(true);
		if (sub_grabber) sub_grabber->SetEnabled(true);

		bMainSignal = false;
		pControl->Run();

		HANDLE handles[2] = {main_grabber->hDataParsed, main_grabber->hEventDisabled};
		SetEvent(main_grabber->hDataReady);
		DWORD wait_res = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (wait_res == WAIT_OBJECT_0) {
			// If we made it - exit loop.
			if (lPos == 0 || main_grabber->tmLastFrame < (main_grabber->m_AvgTimePerFrame / 2))
				break;
			// Ok. Continuing seeking one frame back.
			ResetEvent(main_grabber->hDataParsed);
			lPos -= main_grabber->m_AvgTimePerFrame;
			skip_frames++;
		}
		else if (wait_res == WAIT_OBJECT_0 + 1) {
			// Seek out of the graph! Trying to seek back.
			lPos -= FRAME_SEEKBACKFRAMECOUNT * main_grabber->m_AvgTimePerFrame;
			skip_frames += FRAME_SEEKBACKFRAMECOUNT;
		}
		else
			env->ThrowError("Error %08x occurred at WaitForSingleObject API function", wait_res);
	}
	if (skip_frames < 0)
		env->ThrowError("Error occurred at seeking process");
	current_frame_number = framenumber - skip_frames;
}

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
	bool bMainSignal = true;
	bool bSubSignal = true;

    if (n != current_frame_number && n != current_frame_number+1) {
        SeekToFrame(n, bMainSignal, env);
		int skip_frames = n - current_frame_number;
		while (skip_frames-- > 0) {
			DropFrame(main_grabber, env, bMainSignal);
			if (sub_grabber) DropFrame(sub_grabber, env);
			bMainSignal = true;
		}
		current_frame_number = n-1;
    }

	// Check that we really need to dump the frame
	if (n == current_frame_number) {
		bMainSignal = false;
		bSubSignal = false;
	}
	else {
		current_frame_number++;
		if (main_grabber->GetEnabled())
			pControl->Run();

		// Sync running states
		if (!main_grabber->GetEnabled() && sub_grabber) 
			sub_grabber->SetEnabled(false);
		if (sub_grabber && !sub_grabber->GetEnabled()) 
			main_grabber->SetEnabled(false);
	}

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
			new FrameHolder(frame_vi, ((params & SP_SWAPVIEWS) == 0) ? frame_main : frame_sub), 
			new FrameHolder(frame_vi, ((params & SP_SWAPVIEWS) == 0) ? frame_sub : frame_main), 
			(params & SP_HORIZONTALSTACK) != 0) -> GetFrame(0, env);
	else
		return frame_main;
}
