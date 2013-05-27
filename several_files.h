#pragma once
#include "graph_creation.h"

class SSIFSourceExt: public IClip {
	SSIFSource *cfile;
	PClip cfileclip_holder;
	int cfile_idx;
	VideoInfo vi;
	vector<string> files_names;
	vector<int> framenum_offsets;
	IScriptEnvironment *env;

	bool bLeft, bRight, bHorizontalStack, bCreateIndex;
	int iSwapViews;

	void AddFileNext(const string& name, int frames, bool load);
	void ChangeCurrentFile(int new_idx);
public:
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

	SSIFSourceExt(AVSValue& args, IScriptEnvironment* env);
	virtual ~SSIFSourceExt();

	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

	bool WINAPI GetParity(int n) { return false; }
	void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
	const VideoInfo& WINAPI GetVideoInfo() { return vi; }
	void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};
