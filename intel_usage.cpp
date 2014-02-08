#include "stdafx.h"
#include "utils.h"
#include "intel_usage.h"

extern string program_path;

#define FILTER_NAME "ssifSource"
#define FRAME_START -1
#define FRAME_BLACK -2

std::string IntToStr(int a) {
	char buffer[32];
	_itoa_s(a, buffer, 32, 10);
	return buffer;
}

string SSIFSource::MakePipeName(int id, const string& name) {
	return (string)"\\\\.\\pipe\\bluray" + IntToStr(id) + "\\" + name;
}

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal) {
	const char* arg_names[2] = {NULL, NULL};
	AVSValue args[2] = {a, b};
	return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

PVideoFrame FrameStack(IScriptEnvironment* env, VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal) {
	return ClipStack(env, new FrameHolder(vi, a), new FrameHolder(vi, b), horizontal)->GetFrame(0, env);
}

void SSIFSource::InitVariables() {
	// VideoType
    frame_vi.width = data.dim_width;
    frame_vi.height = data.dim_height;
	frame_vi.fps_numerator = 24000;
	frame_vi.fps_denominator = 1001;
	frame_vi.pixel_type = VideoInfo::CS_I420;
	frame_vi.num_frames = data.frame_count;
    frame_vi.audio_samples_per_second = 0;

    vi = frame_vi;
	if ((data.show_params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
		((data.show_params & SP_HORIZONTAL) ? vi.width : vi.height) *= 2;
	
	last_frame = FRAME_BLACK;
	
#ifdef _DEBUG
	AllocConsole();
#endif
	memset(&SI, 0, sizeof(STARTUPINFO));
	SI.cb = sizeof(SI);
	SI.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
#ifdef _DEBUG
	SI.wShowWindow = SW_SHOWNORMAL;
#else
	SI.wShowWindow = SW_HIDE;
#endif

	memset(&PI1, 0, sizeof(PROCESS_INFORMATION));
	PI1.hProcess = INVALID_HANDLE_VALUE;
	memset(&PI2, 0, sizeof(PROCESS_INFORMATION));
	PI2.hProcess = INVALID_HANDLE_VALUE;
	memset(&PI3, 0, sizeof(PROCESS_INFORMATION));
	PI3.hProcess = INVALID_HANDLE_VALUE;

	frLeft = NULL;
    frRight = NULL;
	dupThread1 = NULL;
	dupThread2 = NULL;
	unic_number = rand();

	pipes_over_warning = false;
}

void SSIFSource::InitDemuxer() {
/*
	data.left_264 = MakePipeName(unic_number, "left1.h264");
	data.right_264 = MakePipeName(unic_number, "right1.h264");

	string
		name_demux = program_path + "eac3to.exe",
		s_demux_left_write = MakePipeName(unic_number, "left.h264"),
		s_demux_right_write = MakePipeName(unic_number, "right.h264"),
		cmd_demux_left = "\"" + name_demux + "\" \"" + data.ssif_file + "\" " + IntToStr(data.left_track) + ":" + s_demux_left_write,
		cmd_demux_right = "\"" + name_demux + "\" \"" + data.ssif_file + "\" " + IntToStr(data.right_track) + ":" + s_demux_right_write;

	dupThread1 = new PipeDupThread(s_demux_left_write.c_str(), data.left_264.c_str());
	dupThread2 = new PipeDupThread(s_demux_right_write.c_str(), data.right_264.c_str());
	if (
		!CreateProcessA(name_demux.c_str(), const_cast<char*>(cmd_demux_left.c_str()), NULL, NULL, false, 
			CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, NULL, NULL, &SI, &PI1) ||
		!CreateProcessA(name_demux.c_str(), const_cast<char*>(cmd_demux_right.c_str()), NULL, NULL, false, 
			CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, NULL, NULL, &SI, &PI2))
	{
		throw (string)"Error while launching " + name_demux + "\n";
	}
*/
}

void SSIFSource::InitDecoder() {
	bool flag_mvc = (data.show_params & SP_RIGHTVIEW) != 0;
    string
		name_decoder = program_path + "..\\bin\\sample_decode.exe",
		s_dec_left_write = MakePipeName(unic_number, "output_0.yuv"),
		s_dec_right_write = MakePipeName(unic_number, "output_1.yuv"),
		s_dec_out = MakePipeName(unic_number, "output"),
		cmd_decoder = "\"" + name_decoder + "\" " +
			(flag_mvc ? "mvc" : "h264") +
			" -i \"" + data.h264muxed + "\" -o " + s_dec_out;

	int framesize = frame_vi.width*frame_vi.height + (frame_vi.width*frame_vi.height/2)&~1;
	if (!flag_mvc)
		s_dec_left_write = s_dec_out;

	frLeft = new FrameSeparator(s_dec_left_write.c_str(), framesize);
	if (flag_mvc)
		frRight = new FrameSeparator(s_dec_right_write.c_str(), framesize);

	if (!CreateProcessA(name_decoder.c_str(), const_cast<char*>(cmd_decoder.c_str()), NULL, NULL, false,
			CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, NULL, &SI, &PI3))
	{
		throw (string)"Error while launching " + name_decoder + "\n";
	}
}

void SSIFSource::InitComplete() {
//	ResumeThread(PI1.hThread);
//	ResumeThread(PI2.hThread);
	ResumeThread(PI3.hThread);
	last_frame = FRAME_START;
}

SSIFSource::SSIFSource(IScriptEnvironment* env, const SSIFSourceParams& data): data(data) {
	InitVariables();

	try {
//		if (!data.ssif_file.empty())
//			InitDemuxer();
		InitDecoder();
		InitComplete();
	}
	catch(const string& obj) {
		env->ThrowError(string(FILTER_NAME ": " + obj).c_str());
	}
}

SSIFSource::~SSIFSource() {
	if (PI3.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI3.hProcess, 0);
//	if (PI2.hProcess != INVALID_HANDLE_VALUE)
//		TerminateProcess(PI2.hProcess, 0);
//	if (PI1.hProcess != INVALID_HANDLE_VALUE)
//		TerminateProcess(PI1.hProcess, 0);
	if (dupThread1 != NULL) 
		delete dupThread1;
	if (dupThread2 != NULL) 
		delete dupThread2;
	if (frLeft != NULL) 
		delete frLeft;
	if (frRight != NULL)
		delete frRight;
}

PVideoFrame SSIFSource::ReadFrame(FrameSeparator* frSep, IScriptEnvironment* env) {
    PVideoFrame res = env->NewVideoFrame(frame_vi);
    if (!frSep->error) {
        frSep->WaitForData();
    } else {
        if (!pipes_over_warning) {
            fprintf(stderr, "\nWARNING: Decoder output finished. Frame separator can't read next frames. "
                "Last frame will be duplicated as long as necessary (%d time(s)).\n", vi.num_frames-last_frame);
            pipes_over_warning = true;
        }
    }
    memcpy(res->GetWritePtr(), (char*)frSep->buffer, frSep->size);
    frSep->DataParsed();
    return res;
}

void SSIFSource::DropFrame(FrameSeparator* frSep) {
    if (!frSep->error)
        frSep->WaitForData();
    frSep->DataParsed();
}

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
	if (last_frame+1 != n || n >= vi.num_frames) {
		string str = "ERROR:\\n"
			"Can't retrieve frame #" + IntToStr(n) + " !\\n";
		if (last_frame >= vi.num_frames-1)
			str += "Video sequence is over. Reload the script.\\n";
		else
			str += "Frame #" + IntToStr(last_frame+1) + " should be the next frame.\\n";
		str += "Note: " FILTER_NAME " filter supports only sequential frame rendering.";
		const char* arg_names1[2] = {0, "color_yuv"};
		AVSValue args1[2] = {this, 0};
		PClip resultClip1 = (env->Invoke("BlankClip", AVSValue(args1,2), arg_names1)).AsClip();

		const char* arg_names2[6] = {0, 0, "lsp", "size", "text_color", "halo_color"};
		AVSValue args2[6] = {resultClip1, AVSValue(str.c_str()), 10, 50, 0xFFFFFF, 0x000000};
		PClip resultClip2 = (env->Invoke("Subtitle", AVSValue(args2,6), arg_names2)).AsClip();
		return resultClip2->GetFrame(n, env);
	}
	last_frame = n;
    
    PVideoFrame left, right;
    if (data.show_params & SP_LEFTVIEW) 
        left = ReadFrame(frLeft, env);
    else
        DropFrame(frLeft);
    if (data.show_params & SP_RIGHTVIEW) 
        right = ReadFrame(frRight, env);

	if ((data.show_params & (SP_LEFTVIEW|SP_RIGHTVIEW)) == (SP_LEFTVIEW|SP_RIGHTVIEW)) {
		if (!(data.show_params & SP_SWAPVIEWS))
			return FrameStack(env, frame_vi, left, right, (data.show_params & SP_HORIZONTAL) != 0);
		else
			return FrameStack(env, frame_vi, right, left, (data.show_params & SP_HORIZONTAL) != 0);
	}
    else if (data.show_params & SP_LEFTVIEW)
        return left;
    else
        return right;
}

AVSValue __cdecl Create_SSIFSource(AVSValue args, void* user_data, IScriptEnvironment* env) {
	SSIFSourceParams data;
	data.h264muxed = args[0].AsString();
	data.frame_count = args[1].AsInt();
	data.dim_width = 1920;
	data.dim_height = 1080;
	data.show_params = 
		(args[2].AsBool(true) ? SP_LEFTVIEW : 0) |
		(args[3].AsBool(true) ? SP_RIGHTVIEW : 0) |
		(args[4].AsBool(false) ? SP_HORIZONTAL : 0) |
		(args[5].AsBool(false) ? SP_SWAPVIEWS : 0);
	if (!(data.show_params & (SP_LEFTVIEW | SP_RIGHTVIEW))) {
        env->ThrowError(FILTER_NAME ": can't show nothing");
    }
	return new SSIFSource(env, data);
}
