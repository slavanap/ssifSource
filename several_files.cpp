#include "stdafx.h"
#include <sstream>
#include "graph_creation.h"
#include "several_files.h"

extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("ssifSource2", "[ssif_file]s[frame_count]i[left_view]b[right_view]b[horizontal_stack]b", SSIFSource::Create, 0);
	env->AddFunction("ssifSource3", "[filelist]s[left_view]b[right_view]b[horizontal_stack]b", SSIFSourceExt::Create, 0);
	return 0;
}

SSIFSourceExt::SSIFSourceExt(AVSValue& args, IScriptEnvironment* env): cfile(NULL), env(env) {
	cfile_idx = -1;
	bLeft = args[1].AsBool(false);
	bRight = args[2].AsBool(true);
	bHorizontalStack = args[3].AsBool(false);

	framenum_offsets.push_back(0);
	std::string files = args[0].AsString();
	while (files.size() > 0) {
		size_t pos = files.find_first_of(';');
		std::string filename = files.substr(0, pos);
		files.erase(0, (pos == std::string::npos) ? pos : (pos+1) );
		if (pos == std::string::npos)
			throw "Frame number does not specified.";
		pos = files.find_first_of(';');
		std::string s_frames = files.substr(0, pos);
		files.erase(0, (pos == std::string::npos) ? pos : (pos+1) );

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

void SSIFSourceExt::AddFileNext(const std::string& name, int frames, bool load) {
	files_names.push_back(name);
	if (frames <= 0) 
		load = true;
	if (load) {
		AVSValue args[5] = {name.c_str(), frames, bLeft, bRight, bHorizontalStack};
		SSIFSource *obj = new SSIFSource(AVSValue(args,5), env);
		frames = obj->GetVideoInfo().num_frames;
		delete obj;
	}
	framenum_offsets.push_back(frames + *framenum_offsets.rbegin());
	vi.num_frames = *framenum_offsets.rbegin();
}

void SSIFSourceExt::ChangeCurrentFile(int new_idx) {
	if (cfile_idx == new_idx)
		return;
	cfileclip_holder = NULL;
	AVSValue args[5] = {files_names[new_idx].c_str(), 
		framenum_offsets[new_idx+1]-framenum_offsets[new_idx], 
		bLeft, bRight, bHorizontalStack};
	cfile = new SSIFSource(AVSValue(args,5), env);
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
