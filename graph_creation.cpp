#include "stdafx.h"
#include "utils.h"
#include "graph_creation.h"

#define FRAME_WAITTIME 120000

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
    hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 1), GetInPin(pDecoder, 1), NULL);
    if (FAILED(hr)) goto lerror;

    ASSERT(left_grabber != NULL);
    hr = pGraph->AddFilter(left_grabber, L"LeftView");
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(pDecoder, 0), GetInPin(left_grabber, 0), NULL);
    if (FAILED(hr)) goto lerror;

    hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pRenderer1);
    if (FAILED(hr)) goto lerror;
    hr = pGraph->AddFilter(pRenderer1, L"Renderer1");
    if (FAILED(hr)) goto lerror;
    hr = pGraph->ConnectDirect(GetOutPin(left_grabber, 0), GetInPin(pRenderer1, 0), NULL);
    if (FAILED(hr)) goto lerror;

//    hr = RenderOutputPins(pGraph, left_grabber);
//    if (FAILED(hr)) goto lerror;
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
//        hr = RenderOutputPins(pGraph, right_grabber);
//        if (FAILED(hr)) goto lerror;
    }

    *ppGraph = pGraph;
    return S_OK;
lerror:
    prog_bag = (IUnknown*)NULL;
    pDecoder = (IUnknown*)NULL;
    pSplitter = (IUnknown*)NULL;
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
    left_grabber = right_grabber = NULL;
    current_frame_number = 0;
    bSuccessCreation = false;
    pGraph = NULL;
    hWindow = NULL;

    params = 0;
    if (args[2].AsBool(false))
        params |= SP_LEFTVIEW;
    if (args[3].AsBool(true))
        params |= SP_RIGHTVIEW;
    if (args[4].AsBool(false))
        params |= SP_HORIZONTALSTACK;

    HRESULT hr = S_OK;
    hWindow = CreateWindow(TEXT("EDIT"), NULL, NULL, 0,0,0,0, HWND_DESKTOP, NULL, hInstance, NULL);

    CoInitialize(NULL);
    {
        USES_CONVERSION;
        left_grabber = new CSampleGrabber(&hr);
        if (params & SP_RIGHTVIEW) right_grabber = new CSampleGrabber(&hr);
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
    pEvent->SetNotifyWindow((OAHWND)hWindow, WM_GRAPH_EVENT, NULL);

    // Run the graph
    hr = pControl->Run();
    if (FAILED(hr)) Throw("Can't run the graph");

    if (left_grabber != NULL && right_grabber != NULL)
        if (left_grabber->m_Width != right_grabber->m_Width || left_grabber->m_Height != right_grabber->m_Height)
            Throw("Differences in output video");

    CSampleGrabber *def_grabber = (left_grabber != NULL) ? left_grabber : right_grabber;
    // copy VideoInfo
    frame_vi = def_grabber->avisynth_vi;
    // Get total number of frames
    {
        CComQIPtr<IMediaSeeking> pSeeking = pGraph;
        LONGLONG lDuration, nTotalFrames;
        pSeeking->GetDuration(&lDuration);
        if (FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME))) {
            nTotalFrames = lDuration / def_grabber->m_AvgTimePerFrame;
            frame_vi.num_frames = args[1].AsInt((int)nTotalFrames);
        } else {
            pSeeking->GetDuration(&nTotalFrames);
            frame_vi.num_frames = (int)nTotalFrames;
        }
    }

    vi = frame_vi;
    if ((params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
        ((params & SP_HORIZONTALSTACK) ? vi.width : vi.height) *= 2;
}

void SSIFSource::Clear() {
    if (pControl) {
        MSG msg;
        pControl->StopWhenReady();
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        long evCode = 0;
        LONG_PTR param1 = 0, param2 = 0;

        HRESULT hr = S_OK;

        // Get the events from the queue.
        while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0)))
        {
            // Invoke the callback.
            if (evCode == EC_COMPLETE) {
                CComQIPtr<IMediaControl>(pGraph)->Stop();
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
    if (left_grabber != NULL) {
        delete left_grabber;
        left_grabber = NULL;
    }
    if (right_grabber != NULL) {
        delete right_grabber;
        right_grabber = NULL;
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

void SSIFSource::DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf) {
    grabber->pData = vf->GetWritePtr();
    SetEvent(grabber->hDataReady);
    WaitForSingleObject(grabber->hDataParsed, INFINITE);
    ResetEvent(grabber->hDataParsed);
    SetEvent(grabber->hDataParsed);
}

void SSIFSource::DropGrabberData(CSampleGrabber *grabber) {
    SetEvent(grabber->hDataReady);
    WaitForSingleObject(grabber->hDataParsed, INFINITE);
    ResetEvent(grabber->hDataParsed);
    SetEvent(grabber->hDataParsed);
}

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
    if (n != current_frame_number)
        return env->NewVideoFrame(vi);
    current_frame_number++;

    //pControl->Run();
    PVideoFrame left, right;
    if (params & SP_LEFTVIEW) {
        left = env->NewVideoFrame(frame_vi);
        DataToFrame(left_grabber, left);
    } else {
        DropGrabberData(left_grabber);
    }
    if (params & SP_RIGHTVIEW) {
        right = env->NewVideoFrame(frame_vi);
        DataToFrame(right_grabber, right);
    }

    if ((params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
        return ClipStack(env, new FrameHolder(frame_vi, left), new FrameHolder(frame_vi, right), (params & SP_HORIZONTALSTACK) != 0)->GetFrame(0, env);
    else if (params & SP_LEFTVIEW)
        return left;
    else
        return right;
}
