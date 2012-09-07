#include "stdafx.h"
#include "utils.h"
#include "grabber.h"

#ifndef _USRDLL

HRESULT CreateGraph(const WCHAR* sFileName, IBaseFilter* left_dumper, IBaseFilter* right_dumper, IGraphBuilder** ppGraph);

int _tmain(int argc, _TCHAR* argv[]) {
    hInstance = GetModuleHandle(NULL);
    IGraphBuilder *pGraph = NULL;

    CoInitialize(NULL);
    CSampleGrabber *left_view = CreateGrabber();
    CSampleGrabber *right_view = CreateGrabber();

    left_view->m_hPipe = CreateFile(TEXT("c:\\0\\left.rgb"), 
        GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    right_view->m_hPipe = CreateFile(TEXT("c:\\0\\right.rgb"), 
        GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FAILED(CreateGraph(L"00001.ssif", static_cast<IBaseFilter*>(left_view), static_cast<IBaseFilter*>(right_view), &pGraph)))
        return -1;
    left_view->Release();
    right_view->Release();

    

    CComQIPtr<IMediaControl>(pGraph)->Run();
    
    // Get total number of frames
    CComQIPtr<IMediaSeeking> pSeeking = pGraph;
    LONGLONG lDuration, nTotalFrames;
    pSeeking->GetDuration(&lDuration);
    if(FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME)))
        nTotalFrames = lDuration / left_view->m_AvgTimePerFrame;
    else
        pSeeking->GetDuration(&nTotalFrames);
    printf("%ld\n", nTotalFrames);

    MSG msg;
    while (GetMessage(&msg, NULL, NULL, NULL)) {
        DispatchMessage(&msg);
    }

    pGraph->Release();
    CoUninitialize();
	return 0;
}

#else

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif
