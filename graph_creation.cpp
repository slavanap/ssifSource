#include "stdafx.h"
#include "utils.h"
#include "graph_creation.h"

#define FRAME_WAITTIME 10000

EXTERN_C const CLSID CLSID_NullRenderer;

LPOLESTR lib_splitter = T2OLE(L"MPEGSplitter.dll");
LPOLESTR lib_decoder = T2OLE(L"CoreAVCDecoder.dll");

HRESULT CreateGraph(const WCHAR* sFileName, IBaseFilter* left_grabber, IBaseFilter* right_grabber, IGraphBuilder** ppGraph) {
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
    pGraph->ConnectDirect(GetOutPin(pSplitter, 1), GetInPin(pDecoder, 1), NULL);
    // Ignore the error here. For 'MVC (Full)' support

    if (left_grabber != NULL) {
        hr = pGraph->AddFilter(left_grabber, L"LeftView");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(pDecoder, 0), GetInPin(left_grabber, 0), NULL);
        if (FAILED(hr)) goto lerror;
    }
    else {
        left_grabber = pDecoder;
    }

    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer1);
    if (FAILED(hr)) goto lerror;
    hr = pGraph->AddFilter(pRenderer1, L"Renderer1");
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(left_grabber, 0), GetInPin(pRenderer1, 0), NULL);
    if (FAILED(hr)) goto lerror;

    if (right_grabber != NULL) {
        hr = pGraph->AddFilter(right_grabber, L"RightView");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(pDecoder, 1), GetInPin(right_grabber, 0), NULL);
        if (FAILED(hr)) goto lerror;

        hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer2);
        if (FAILED(hr)) goto lerror;
        hr = pGraph->AddFilter(pRenderer2, L"Renderer2");
        if (FAILED(hr)) goto lerror;
        hr = pGraph->ConnectDirect(GetOutPin(right_grabber, 0), GetInPin(pRenderer2, 0), NULL);
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


extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
    env->AddFunction("ssifSource2", "[ssif_file]s[frame_count]i[left_view]b[right_view]b[horizontal_stack]b", SSIFSource::Create, 0);
    return 0;
}

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal = false) {
    const char* arg_names[2] = {NULL, NULL};
    AVSValue args[2] = {a, b};
    return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

SSIFSource::SSIFSource(AVSValue& args, IScriptEnvironment* env) {
    main_grabber = sub_grabber = NULL;
    current_frame_number = 0;
    pGraph = NULL;
    hWindow = NULL;
    tmDuration = 0;

    params = 0;
    if (args[2].AsBool(false))
        params |= SP_LEFTVIEW;
    if (args[3].AsBool(true))
        params |= SP_RIGHTVIEW;
    if (args[4].AsBool(false))
        params |= SP_HORIZONTALSTACK;
    if (!(params & (SP_LEFTVIEW | SP_RIGHTVIEW)))
        Throw("No view selected!");

    HRESULT hr = S_OK;
    CoInitialize(NULL);
    {
        USES_CONVERSION;
        CSampleGrabber *left_grabber = NULL, *right_grabber = NULL;
        if (params & SP_LEFTVIEW)
            main_grabber = left_grabber = new CSampleGrabber(&hr);
        if (params & SP_RIGHTVIEW)
            *((main_grabber == NULL) ? &main_grabber : &sub_grabber) = right_grabber = new CSampleGrabber(&hr);
        LPCSTR filename = args[0].AsString();
        hr = CreateGraph(A2W(filename), static_cast<IBaseFilter*>(left_grabber), static_cast<IBaseFilter*>(right_grabber), &pGraph);
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

    // Run the graph
    hr = pControl->Run();
    if (FAILED(hr)) Throw("Can't run the graph");

    if (main_grabber != NULL && sub_grabber != NULL)
        if (main_grabber->m_Width != sub_grabber->m_Width || main_grabber->m_Height != sub_grabber->m_Height)
            Throw("Differences in output video");

    // copy VideoInfo
    frame_vi = main_grabber->avisynth_vi;
    // Get total number of frames
    {
        CComQIPtr<IMediaSeeking> pSeeking = pGraph;
        LONGLONG nTotalFrames;
        pSeeking->GetDuration(&tmDuration);
        nTotalFrames = (tmDuration + main_grabber->m_AvgTimePerFrame/2) / main_grabber->m_AvgTimePerFrame;
        frame_vi.num_frames = args[1].AsInt((int)nTotalFrames);
        logger.log("duration %lld, avg %lld", tmDuration, main_grabber->m_AvgTimePerFrame);
        main_grabber->bLog = true;
    }

    vi = frame_vi;
    if ((params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
        ((params & SP_HORIZONTALSTACK) ? vi.width : vi.height) *= 2;

    ifstream ofsfile("offsets.txt");
    long hres;
    REFERENCE_TIME lStart, lEnd;
    while (ofsfile >> hres >> lStart >> lEnd) {
        if (hres) env->ThrowError("hres not zero");
        offsets.push_back(lStart);
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

        while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
            // Invoke the callback.
            if (evCode == EC_COMPLETE) {
                pControl->Stop();
            }

            // Free the event data.
            hr = pEvent->FreeEventParams(evCode, param1, param2);
            if (FAILED(hr))
                break;
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
    if (!grabber->GetEnabled() || (WaitForSingleObject(grabber->hDataParsed, INFINITE) != WAIT_OBJECT_0)) {
        printf("\nssifSource2: Seek out of graph. %6d frame duplicate added (debug: g%08x m%08x s%08x)\n", 
            current_frame_number-1, grabber, main_grabber, sub_grabber);
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

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
    bool bMainSignal = true;
    if (n != current_frame_number) {
        printf("\nssifSource2: seeking to frame %d (lastframe %d)\n", n, current_frame_number-1);

        CComQIPtr<IMediaSeeking> pSeeking = pGraph;
        REFERENCE_TIME lPos = main_grabber->m_AvgTimePerFrame * n;
        int skip_frames = 0;

        while (1) {
            main_grabber->SetEnabled(false);
            if (sub_grabber) sub_grabber->SetEnabled(false);
            pControl->Pause();

            if (lPos >= tmDuration)
                break;

            HRESULT hr = pSeeking->SetPositions(&lPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
            logger.log("frame %d, pos %d, diff %d, res %x", n, (int)(lPos), 
                (int)((n >= (int)offsets.size()) ? 0 : (offsets[n] - lPos)), hr);

            main_grabber->SetEnabled(true);
            if (sub_grabber) sub_grabber->SetEnabled(true);

            bMainSignal = false;
            pControl->Run();
            ParseEvents();

            SetEvent(main_grabber->hDataReady);
            if (WaitForSingleObject(main_grabber->hDataParsed, FRAME_WAITTIME) != WAIT_OBJECT_0) {
                // Seek out of the graph!
                main_grabber->SetEnabled(false);
                if (sub_grabber) sub_grabber->SetEnabled(false);
                bMainSignal = true;
                skip_frames = 0;
                break;
            }
            if (main_grabber->tmLastFrame < (main_grabber->m_AvgTimePerFrame / 2))
                break;
            ResetEvent(main_grabber->hDataParsed);
            lPos -= main_grabber->m_AvgTimePerFrame;
            skip_frames++;
        }
        while (skip_frames-- > 0) {
            DropFrame(main_grabber, env, bMainSignal);
            if (sub_grabber) DropFrame(sub_grabber, env);
            bMainSignal = true;
        }

        main_grabber->nFrame = n;
        if (sub_grabber) sub_grabber->nFrame = n;
        current_frame_number = n;
    }
    current_frame_number++;
    ParseEvents();

    if (main_grabber->GetEnabled())
        pControl->Run();
    PVideoFrame frame_main, frame_sub;
    frame_main = env->NewVideoFrame(frame_vi);
    DataToFrame(main_grabber, frame_main, env, bMainSignal);
    if (sub_grabber) {
        frame_sub = env->NewVideoFrame(frame_vi);
        DataToFrame(sub_grabber, frame_sub, env);
    }

    if (sub_grabber)
        return ClipStack(env, new FrameHolder(frame_vi, frame_main), new FrameHolder(frame_vi, frame_sub), 
            (params & SP_HORIZONTALSTACK) != 0)->GetFrame(0, env);
    else
        return frame_main;
}
