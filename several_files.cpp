#include "stdafx.h"
#include "several_files.h"

#define SSIFSOURCE3_PLUGIN "ssifSource3"
#define FORMAT_PRINTMESSAGE(x) SSIFSOURCE3_PLUGIN ": " x "\n"

SSIFSourceExt::SSIFSourceExt(AVSValue& args, IScriptEnvironment* env): cfile(NULL), env(env) {
	cfile_idx = -1;
	bLeft = args[1].AsBool(false);
	bRight = args[2].AsBool(true);
	bHorizontalStack = args[3].AsBool(false);
	iSwapViews = args[4].AsInt(-1);
	bCreateIndex = args[5].AsBool(false);

	framenum_offsets.push_back(0);
	string files = args[0].AsString();
	while (files.size() > 0) {
		size_t pos = files.find_first_of(';');
		string filename = files.substr(0, pos);
		files.erase(0, (pos == string::npos) ? pos : (pos+1) );
		if (pos == string::npos)
			throw "Frame number does not specified.";
		pos = files.find_first_of(';');
		string s_frames = files.substr(0, pos);
		files.erase(0, (pos == string::npos) ? pos : (pos+1) );

		int frames;
		if (!StrToIntExA(s_frames.c_str(), STIF_DEFAULT, &frames))
			throw "Frame number is not a number. Use 0 value if you want to force framenumber detection.";
		AddFileNext(filename, frames, frames == 0);
	}
	ChangeCurrentFile(0);
	vi = cfile->GetVideoInfo();
	vi.num_frames = *framenum_offsets.rbegin();
}

SSIFSourceExt::~SSIFSourceExt() {
	cfileclip_holder = NULL;
}

void SSIFSourceExt::AddFileNext(const string& name, int frames, bool load) {
	files_names.push_back(name);
	if (frames <= 0) 
		load = true;
	printf(FORMAT_PRINTMESSAGE("adding file %s with %d frames to sequences list. Have to load flag is %s"), 
		name.c_str(), frames, load ? "TRUE" : "FALSE");
	if (load) {
		AVSValue args[7] = {name.c_str(), frames, bLeft, bRight, bHorizontalStack, iSwapViews, bCreateIndex};
		SSIFSource *obj = new SSIFSource(AVSValue(args, sizeof(args)/sizeof(AVSValue)), env);
		frames = obj->GetVideoInfo().num_frames;
		if (cfile_idx < 0) {
			cfileclip_holder = cfile = obj;
			cfile_idx = files_names.size()-1;
		}
		else
			delete obj;
	}
	framenum_offsets.push_back(frames + *framenum_offsets.rbegin());
	vi.num_frames = *framenum_offsets.rbegin();
}

void SSIFSourceExt::ChangeCurrentFile(int new_idx) {
	if (cfile_idx == new_idx)
		return;
	cfileclip_holder = NULL;
	AVSValue args[7] = {files_names[new_idx].c_str(), 
		framenum_offsets[new_idx+1]-framenum_offsets[new_idx], 
		bLeft, bRight, bHorizontalStack, iSwapViews, bCreateIndex};
	cfile = new SSIFSource(AVSValue(args, sizeof(args)/sizeof(AVSValue)), env);
	cfileclip_holder = cfile;
	cfile_idx = new_idx;
}

PVideoFrame SSIFSourceExt::GetFrame(int n, IScriptEnvironment* env) {
	int idx = cfile_idx;
	while (n >= framenum_offsets[idx+1])
		++idx;
	while (framenum_offsets[idx] > n)
		--idx;
	ChangeCurrentFile(idx);
	return cfile->GetFrame(n-framenum_offsets[idx], env);
}

AVSValue __cdecl SSIFSourceExt::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
	SSIFSourceExt *obj = NULL;
	try {
		obj = new SSIFSourceExt(args, env);
	} catch(const char* str) {
		env->ThrowError("%s", str);
	}
	return obj;
}
