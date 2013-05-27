#include "stdafx.h"
#include "utils.h"
#include "ldecod_usage.h"
#include "coreavc_usage.h"

#define PATH_BUFFER_LENGTH 1024
string program_path;

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
}

void SSIFSource::InitDecoder() {
    string
		name_decoder = program_path + "ldecod.exe",

    	s_dec_left_write = MakePipeName(unic_number, "10.yuv"),
		s_dec_right_write = MakePipeName(unic_number, "11.yuv"),
		s_dec_out = MakePipeName(unic_number, "1.yuv"),

		//  WriteStreams=1 - right only,  WriteStreams=0 - left only,  WriteStreams=-1 - both
		cmd_decoder = "\"" + name_decoder + "\" -p InputFile=\"" + data.left_264 + "\" -p InputFile2=\"" + data.right_264 + "\" "
			"-p OutputFile=\"" + s_dec_out + "\" "
            "-p WriteUV=0 -p FileFormat=0 -p RefOffset=0 -p POCScale=2 -p DisplayDecParams=1 -p ConcealMode=0 "
            "-p RefPOCGap=2 -p POCGap=2 -p Silent=1 -p IntraProfileDeblocking=1 -p DecFrmNum=0 -p DecodeAllLayers=1 " 
            "-p WriteStreams=-1";

	int framesize = frame_vi.width*frame_vi.height + (frame_vi.width*frame_vi.height/2)&~1;
    frLeft = new FrameSeparator(s_dec_left_write.c_str(), framesize);
    frRight = new FrameSeparator(s_dec_right_write.c_str(), framesize);

	// CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP
	if (!CreateProcessA(name_decoder.c_str(), const_cast<char*>(cmd_decoder.c_str()), NULL, NULL, false,
			CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, NULL, &SI, &PI3))
	{
		throw (string)"Error while launching " + name_decoder + "\n";
	}
}

void SSIFSource::InitComplete() {
	ResumeThread(PI1.hThread);
	ResumeThread(PI2.hThread);
	ResumeThread(PI3.hThread);
	last_frame = FRAME_START;
}

SSIFSource* SSIFSource::Create(const SSIFSourceParams& data, IScriptEnvironment* env) {
	SSIFSource *res = new SSIFSource();
	res->data = data;
	res->InitVariables();

	try {
		if (!data.ssif_file.empty())
			res->InitDemuxer();
		res->InitDecoder();
		res->InitComplete();
	}
	catch(const string& obj) {
		delete res;
		env->ThrowError(string(FILTER_NAME ": " + obj).c_str());
	}

	return res;
}

SSIFSource::~SSIFSource() {
	if (PI3.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI3.hProcess, 0);
	if (PI2.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI2.hProcess, 0);
	if (PI1.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI1.hProcess, 0);
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
            printf("\nWARNING: Decoder output finished. Frame separator can't read next frames. "
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
    else
        DropFrame(frRight);

    if ((data.show_params & (SP_LEFTVIEW|SP_RIGHTVIEW)) == (SP_LEFTVIEW|SP_RIGHTVIEW))
        return ClipStack(env, new FrameHolder(frame_vi, left), new FrameHolder(frame_vi, right), (data.show_params & SP_HORIZONTAL) != 0)->GetFrame(0, env);
    else if (data.show_params & SP_LEFTVIEW)
        return left;
    else
        return right;
}

AVSValue __cdecl Create_SSIFSource(AVSValue args, void* user_data, IScriptEnvironment* env) {
	SSIFSourceParams data;
	if (args[0].Defined()) {
		data.ssif_file = args[0].AsString();
		data.left_track = args[4].AsInt(data.left_track);
		data.right_track = args[5].AsInt(data.right_track);
	} else 
	if (args[6].Defined() && args[7].Defined()) {
		if (args[4].Defined() || args[5].Defined()) {
			env->ThrowError(FILTER_NAME ": tracks specification is forbidden for separated H.264 files.");
		}
		data.left_264 = args[6].AsString();
		data.right_264 = args[7].AsString();
	} else {
		env->ThrowError(FILTER_NAME ": appropriate input files is not specified.");
	}

	data.dim_width = args[1].AsInt(0);
	data.dim_height = args[2].AsInt(0);
	data.frame_count = args[3].AsInt(0);
	if (data.dim_width <= 0 || data.dim_height <= 0 || data.frame_count <= 0) {
		env->ThrowError(FILTER_NAME ": check width, height and frame_count parameters.");
	}
    data.show_params = args[8].AsInt(data.show_params);
    if (!(data.show_params & SP_MASK)) {
        env->ThrowError(FILTER_NAME ": can't show nothing. Check show_params.");
    }        
	return SSIFSource::Create(data, env);
}

bool SSIFSource::DLLInit() {
	char buffer[PATH_BUFFER_LENGTH];
	int res;
	srand((unsigned int)time(NULL));
	res = GetModuleFileNameA(hInstance, buffer, PATH_BUFFER_LENGTH);
	if (res == 0) {
		MessageBoxA(HWND_DESKTOP, "Can not retrieve program path", NULL, MB_ICONERROR | MB_OK);
		return false;
	}
	program_path = buffer;
	program_path.erase(program_path.rfind("\\")+1);
	return true;
}
