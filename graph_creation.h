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
    CSampleGrabber *left_grabber, *right_grabber;
    VideoInfo vi, frame_vi;
    int params;
    int current_frame_number;
    IGraphBuilder *pGraph;
    CComQIPtr<IMediaEventEx> pEvent;
    CComQIPtr<IMediaControl> pControl;
    HWND hWindow;

    void Clear();
    void Throw(const char* str) {
        Clear();
        throw str;
    }
    void DataToFrame(CSampleGrabber *grabber, PVideoFrame& vf, IScriptEnvironment* env);
public:
	SSIFSource(AVSValue& args, IScriptEnvironment* env);
    virtual ~SSIFSource();

	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
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
