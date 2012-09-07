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

SSIFSource::SSIFSource() {
    hOpFinished = hThreadDestroy = NULL;
    lpThreadError = NULL;
    hrThreadCode = E_UNEXPECTED;
    args = NULL;
    env = NULL;
    hThread = NULL;

    left_grabber = right_grabber = NULL;
    memset(&vi, 0, sizeof(VideoInfo));
}

SSIFSource::~SSIFSource() {
    if (hThread != NULL) SetEvent(hThreadDestroy);
    if (WaitForSingleObject(hOpFinished, 5000*100) != WAIT_OBJECT_0) {
        TerminateThread(hThread, 0xFFFFFFFF);
        // Can't free graph & interfaces created in a foreign thread
    }
    SAFE_CloseHandle(hOpFinished);
    SAFE_CloseHandle(hThreadDestroy);
}


AVSValue __cdecl SSIFSource::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    DWORD threadId;
    SSIFSource *self = new SSIFSource();

    self->current_frame_number = 0;

    self->params = 0;
    if (args[2].AsBool(false))
        self->params |= SP_LEFTVIEW;
    if (args[3].AsBool(true))
        self->params |= SP_RIGHTVIEW;
    if (args[4].AsBool(false))
        self->params |= SP_HORIZONTALSTACK;

    self->hOpFinished = CreateEvent(NULL, TRUE, FALSE, NULL);
    self->hThreadDestroy = CreateEvent(NULL, TRUE, FALSE, NULL);
    self->args = &args;
    self->env = env;
    self->lpThreadError = NULL;
    self->hrThreadCode = E_UNEXPECTED;
    self->hThread = CreateThread(NULL, 0, &GrabberThreadFunc, self, 0, &threadId);

    if (self->hThread == NULL) {
        self->ThreadSetError("Can't create a new thread!", GetLastError(), false);
        goto error;
    }
    // Wait for thread init
    if (WaitForSingleObject(self->hOpFinished, INFINITE) != WAIT_OBJECT_0) {
        self->ThreadSetError("Something strange happen", GetLastError(), false);
        goto error;
    }
    if (FAILED(self->hrThreadCode))
        goto error;
    ResetEvent(self->hOpFinished);

    // message loop in child thread already started
    self->vi = self->frame_vi;
    if ((self->params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
        ((self->params & SP_HORIZONTALSTACK) ? self->vi.width : self->vi.height) *= 2;

    // creation finished
    return self;
error:
    {
        CHAR buffer[1024];
        LPSTR msg = self->lpThreadError;
        HRESULT code = self->hrThreadCode;
        ErrorMessageA(self->hrThreadCode, buffer, 1024);
        delete self;
        env->ThrowError("%s (0x%08x: %s)", msg, code, buffer);
    }
    return NULL;
}

DWORD WINAPI SSIFSource::GrabberThreadFunc(LPVOID arg) {
    SSIFSource *self = (SSIFSource*)arg;
    HRESULT hr = S_OK;
    IGraphBuilder *pGraph = NULL;
    CComQIPtr<IMediaEventEx> pEvent;
    HWND hWindow = CreateWindow(TEXT("EDIT"), NULL, NULL, 0,0,0,0, HWND_DESKTOP, NULL, hInstance, NULL);

    CoInitialize(NULL);
    //CoInitializeEx(0, COINIT_MULTITHREADED);
    {
        USES_CONVERSION;
        self->left_grabber = CreateGrabber();
        if (self->params & SP_RIGHTVIEW) self->right_grabber = CreateGrabber();
        LPCSTR filename = (*self->args)[0].AsString();
        hr = CreateGraph(A2W(filename), 
            static_cast<IBaseFilter*>(self->left_grabber), 
            static_cast<IBaseFilter*>(self->right_grabber), 
            &pGraph);
        if (FAILED(hr)) {
            if (self->left_grabber != NULL) delete self->left_grabber;
            if (self->right_grabber != NULL) delete self->right_grabber;
            self->ThreadSetError("Can't create the graph", hr);
            return 0;
        }
    }
    pEvent = pGraph;
    if (!pEvent) {
        self->ThreadSetError("Can't retrieve IMediaEventEx interface", hr, false);
        goto error;
    }
    pEvent->SetNotifyWindow((OAHWND)hWindow, WM_GRAPH_EVENT, NULL);

    // Run the graph
    hr = CComQIPtr<IMediaControl>(pGraph)->Run();
    if (FAILED(hr)) {
        self->ThreadSetError("Can't run the graph", hr, false);
        goto error;
    }

    if (self->left_grabber != NULL && self->right_grabber != NULL) {
        CSampleGrabber &lg = *self->left_grabber, &rg = *self->right_grabber;
        if (lg.m_Width != rg.m_Width || lg.m_Height != rg.m_Height || lg.m_SampleSize != rg.m_SampleSize) {
            self->ThreadSetError("Differences in output video", E_UNEXPECTED, false);
            goto error;
        }
    }

    CSampleGrabber *def_grabber = (self->left_grabber != NULL) ? self->left_grabber : self->right_grabber;
    // copy VideoInfo
    self->frame_vi = def_grabber->avisynth_vi;
    // Get total number of frames
    {
        CComQIPtr<IMediaSeeking> pSeeking = pGraph;
        LONGLONG lDuration, nTotalFrames;
        pSeeking->GetDuration(&lDuration);
        if (FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME))) {
            nTotalFrames = lDuration / def_grabber->m_AvgTimePerFrame;
            self->frame_vi.num_frames = (*self->args)[1].AsInt((int)nTotalFrames);
        } else {
            pSeeking->GetDuration(&nTotalFrames);
            self->frame_vi.num_frames = (int)nTotalFrames;
        }
    }

    // let the main thread continue
    self->ThreadSetError(NULL, S_OK);

    while (1) {
        switch(::MsgWaitForMultipleObjects(1, &self->hThreadDestroy, FALSE, INFINITE, QS_ALLINPUT)) {
            case (WAIT_OBJECT_0 + 1):
                MSG msg;
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    if (msg.message == WM_GRAPH_EVENT) {
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
                    else if (msg.message == WM_QUIT) {
                        goto cleanup;
                    }
                    else {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
                break;
            case WAIT_OBJECT_0:
                CComQIPtr<IMediaControl>(pGraph)->StopWhenReady();
                PostMessage(NULL, WM_QUIT, 0, 0);
                break;
            default:
                self->ThreadSetError("Waiting error", GetLastError(), false);
                goto error;
        }
    }

cleanup:
    self->ThreadSetError(NULL, S_OK, false);
error:
    pEvent->SetNotifyWindow(NULL, NULL, NULL);
    pEvent = (LPUNKNOWN)NULL;
    DestroyWindow(hWindow);
    if (self->left_grabber != NULL) self->left_grabber->CloseSyncHandles();
    if (self->right_grabber != NULL) self->right_grabber->CloseSyncHandles();
    CComQIPtr<IMediaControl>(pGraph)->Stop();
    pGraph->Release();
    CoUninitialize();
    self->ThreadSetError();
    return 0;
}

PVideoFrame Black(PClip clip, IScriptEnvironment* env) {
    const char* arg_names1[2] = {0, "color"};
    AVSValue args1[2] = {clip, 0};
    PClip resultClip1 = (env->Invoke("BlankClip", AVSValue(args1,2), arg_names1)).AsClip();
    return resultClip1->GetFrame(0, env);
}

void SSIFSource::DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf, IScriptEnvironment* env) {
    BYTE* dst = vf->GetWritePtr();
    logger.log("M %6d 0x%08x: WaitFor(hDataReady)", current_frame_number-1, grabber);
    if (WaitForSingleObject(grabber->hDataReady, FRAME_WAITTIME) != WAIT_OBJECT_0) {
        vf = Black(new FrameHolder(frame_vi, PVideoFrame()), env);
        return;
    }
    logger.log("M %6d 0x%08x: ResetEvent(hDataReady)", current_frame_number-1, grabber);
    ResetEvent(grabber->hDataReady);
    memcpy(dst, grabber->pData, grabber->m_FrameSize);
    logger.log("M %6d 0x%08x: SetEvent(hDataParsed)", current_frame_number-1, grabber);
    SetEvent(grabber->hDataParsed);
}

void SSIFSource::DropGrabberData(CSampleGrabber *grabber) {
    logger.log("M %6d 0x%08x: WaitFor(hDataReady)", current_frame_number-1, grabber);
    WaitForSingleObject(grabber->hDataReady, FRAME_WAITTIME);
    logger.log("M %6d 0x%08x: ResetEvent(hDataReady)", current_frame_number-1, grabber);
    ResetEvent(grabber->hDataReady);
    logger.log("M %6d 0x%08x: SetEvent(hDataParsed)", current_frame_number-1, grabber);
    SetEvent(grabber->hDataParsed);
}

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal = false) {
    const char* arg_names[2] = {NULL, NULL};
    AVSValue args[2] = {a, b};
    return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
    if (n != current_frame_number)
        return Black(this, env);
    current_frame_number++;

    PVideoFrame left, right;
    if (WaitForSingleObject(hThread, 0) != WAIT_TIMEOUT)
        env->ThrowError("The graph suddenly has been closed");
    if (params & SP_LEFTVIEW) {
        left = env->NewVideoFrame(frame_vi);
        DataToFrame(left_grabber, left, env);
    } else {
        DropGrabberData(left_grabber);
    }
    if (params & SP_RIGHTVIEW) {
        right = env->NewVideoFrame(frame_vi);
        DataToFrame(right_grabber, right, env);
    }

    if ((params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
        return ClipStack(env, new FrameHolder(frame_vi, left), new FrameHolder(frame_vi, right), (params & SP_HORIZONTALSTACK) != 0)->GetFrame(0, env);
    else if (params & SP_LEFTVIEW)
        return left;
    else
        return right;
}
