#pragma once

#include "avisynth.h"

class MPLSSource: public IClip {
	int current_index;
	vector<string> files_names;
	vector<int> frame_offsets;
	PClip current_clip;
	string ssif_path;
	bool flag_mvc;
	bool flag_swapviews;

	AVSValue plugin_params;
	VideoInfo vi;

	void ChangeCurrentFile(int new_idx, IScriptEnvironment* env);
public:
	MPLSSource(IScriptEnvironment* env, AVSValue args);
	virtual ~MPLSSource();

	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

	bool WINAPI GetParity(int n) { return false; }
	void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
	const VideoInfo& WINAPI GetVideoInfo() { return vi; }
	void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};

AVSValue __cdecl Create_MPLSSource(AVSValue args, void* user_data, IScriptEnvironment* env);
