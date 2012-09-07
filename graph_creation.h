#pragma once
#include "avisynth.h"
#include "grabber.h"

const UINT WM_GRAPH_EVENT = WM_APP + 1;

enum ShowParameters {
    SP_LEFTVIEW        = 0x01,
    SP_RIGHTVIEW       = 0x02,
    SP_HORIZONTALSTACK = 0x04,
    SP_MASK            = 0x07
};

class SSIFSource: public IClip {
    HANDLE hOpFinished, hThreadDestroy;
    LPSTR lpThreadError;
    HRESULT hrThreadCode;
    AVSValue *args;
    IScriptEnvironment *env;
    HANDLE hThread;
    
    CSampleGrabber *left_grabber, *right_grabber;
    VideoInfo vi, frame_vi;
    int params;
    int current_frame_number;

    static DWORD WINAPI GrabberThreadFunc(LPVOID arg);
    void ThreadSetError(LPSTR lpMessage, HRESULT hrCode, bool bEvent = true) {
        lpThreadError = lpMessage;
        hrThreadCode = hrCode;
        if (bEvent) SetEvent(hOpFinished);
    }
    void ThreadSetError() {
        SetEvent(hOpFinished);
    }
    SSIFSource();
    void DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf);
    void DropGrabberData(CSampleGrabber *grabber);
public:
    static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
    virtual ~SSIFSource();

    PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

    bool WINAPI GetParity(int n) { return false; }
    void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
    const VideoInfo& WINAPI GetVideoInfo() { return vi; }
    void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};

// class for holding one frame
class FrameHolder: public IClip {
    PVideoFrame vf;
    const VideoInfo vi;
public:
    FrameHolder(const VideoInfo& vi, PVideoFrame& vf): vi(vi), vf(vf) {}
    PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) { return vf; }
    bool WINAPI GetParity(int n) { return false; }
    void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
    const VideoInfo& WINAPI GetVideoInfo() { return vi; }
    void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};
