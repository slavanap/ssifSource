#include "stdafx.h"
#include "mpls_source.h"
#include "mpls_parse.h"

void MPLSSource::ChangeCurrentFile(int new_idx, IScriptEnvironment* env) {
	if (current_index == new_idx)
		return;
	current_clip = NULL;
	printf("\nChanging current sequence from %s to %s\n",
		(current_index < 0) ? "none" : files_names[current_index].c_str(),
		files_names[new_idx].c_str());

	AVSValue args[9] = {files_names[new_idx].c_str(), frame_offsets[new_idx+1]-frame_offsets[new_idx]};
	for(int i=2; i<9; ++i)
		args[i] = plugin_params[i];
	if (!args[5].Defined())
		args[5] = flag_swapviews;
	current_clip = (env->Invoke("ssifSource", AVSValue(args,9))).AsClip();
	current_index = new_idx;
}

PVideoFrame MPLSSource::GetFrame(int n, IScriptEnvironment* env) {
	int idx = current_index;
	while (n >= frame_offsets[idx+1])
		++idx;
	while (frame_offsets[idx] > n)
		--idx;
	ChangeCurrentFile(idx, env);
	return current_clip->GetFrame(n-frame_offsets[idx], env);
}

string extract_path(string filename) {
	size_t pos = filename.rfind("\\");
	if (pos == string::npos)
		return string();
	filename.erase(pos+1);
	return filename;
}

BOOL DirectoryExists(LPCSTR szPath){
	DWORD dwAttrib = GetFileAttributesA(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

MPLSSource::MPLSSource(IScriptEnvironment* env, AVSValue args) {
	// copy plug-in arguments for later usage
	AVSValue *values = new AVSValue[args.ArraySize()];
	for(int i=0; i<args.ArraySize(); ++i)
		values[i] = args[i];
	plugin_params = AVSValue(values, args.ArraySize());

	// currently loaded file is undefined
	current_index = -1;

	flag_mvc = false;

	string mpls_filename = args[0].AsString();
	string mpls_path = extract_path(mpls_filename);
	ssif_path = args[1].Defined() ? args[1].AsString() : mpls_path + "..\\STREAM\\";
	if (DirectoryExists((ssif_path + "SSIF").c_str())) {
		flag_mvc = true;
		ssif_path += "SSIF\\";
	}

	mpls_file_t mpls_file = init_mpls(const_cast<char*>(mpls_filename.c_str()));
	// swap views auto-detection
	flag_swapviews = (mpls_file.data[0x38] & 0x10) != 0;
	printf("Swap views flag value is %s\n", flag_swapviews ? "true" : "false");
	// parse playlist
	playlist_t playlist_base = create_playlist_t();
	parse_stream_clips(&mpls_file, &playlist_base);
	print_stream_clips_header(&playlist_base);
	print_stream_clips(&playlist_base);

	// initialize files_names, framenum_offsets, vi.num_frames
	playlist_t *playlist = &playlist_base;
	stream_clip_t* clip = playlist->stream_clip_list.first;
	vi.num_frames = 0;
	while (clip != NULL) {
		int currect_framecount = (int)((double)clip->raw_duration * 24 / (45 * 1001) + 0.5);
		printf("MPLSSource: adding file %s with %d frames to sequences list.\n", clip->filename, currect_framecount);

		string filename = clip->filename;
		filename.erase(filename.length()-4);
		filename = ssif_path + filename + (flag_mvc ? "SSIF" : "M2TS");
		files_names.push_back(filename);

		frame_offsets.push_back(vi.num_frames);
		vi.num_frames += currect_framecount;
		clip = clip->next;
	}
	frame_offsets.push_back(vi.num_frames);
	free_playlist_members(&playlist_base);
	free_mpls_file_members(&mpls_file);

	ChangeCurrentFile(0, env);
	vi = current_clip->GetVideoInfo();
	vi.num_frames = *frame_offsets.rbegin();
}

MPLSSource::~MPLSSource() {
	delete &plugin_params[0];
	current_clip = NULL;
}

AVSValue __cdecl Create_MPLSSource(AVSValue args, void* user_data, IScriptEnvironment* env) {
	MPLSSource *obj = NULL;
	try {
		obj = new MPLSSource(env, args);
	} catch(const char* str) {
		env->ThrowError("%s", str);
	}
	return obj;
}
