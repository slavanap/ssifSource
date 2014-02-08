#pragma once
#include "avisynth.h"
#include "pipe_tools.h"

enum ShowParameters {
    SP_LEFTVIEW   = 0x01,
    SP_RIGHTVIEW  = 0x02,
    SP_HORIZONTAL = 0x04,
	SP_SWAPVIEWS  = 0x08,
    SP_MASK       = 0x0F
};

struct SSIFSourceParams {
	string h264muxed;
	string left_264;
	string right_264;
	int dim_width;
	int dim_height;
	int frame_count;
    int show_params;

	SSIFSourceParams(): dim_width(0), dim_height(0), frame_count(0), show_params(SP_LEFTVIEW|SP_RIGHTVIEW)
	{
	}
};

class SSIFSource: public IClip {
private:
	SSIFSourceParams data;
	VideoInfo vi, frame_vi;
	int last_frame;
	STARTUPINFOA SI;
	PROCESS_INFORMATION PI1, PI2, PI3;
	FrameSeparator *frLeft, *frRight;
	PipeDupThread *dupThread1, *dupThread2;
	int unic_number;
	bool pipes_over_warning;

	void InitVariables();
	void InitDemuxer();
	void InitDecoder();
	void InitComplete();
    PVideoFrame ReadFrame(FrameSeparator* frSep, IScriptEnvironment* env);
    void DropFrame(FrameSeparator* frSep);

	static string MakePipeName(int id, const string& name);
	SSIFSource(const SSIFSource& obj) {}
public:
	SSIFSource(IScriptEnvironment* env, const SSIFSourceParams& data);
	virtual ~SSIFSource();

	// Avisynth virtual functions
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

    bool WINAPI GetParity(int n) { return false; }
    void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
    const VideoInfo& WINAPI GetVideoInfo() { return vi; }
    void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};

AVSValue __cdecl Create_SSIFSource(AVSValue args, void* user_data, IScriptEnvironment* env);



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
