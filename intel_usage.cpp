#include "stdafx.h"
#include "utils.h"
#include "dump_filter.h"
#include "intel_usage.h"


// paths for binaries for debugging
#ifdef _DEBUG
#define PATH_SPLITTER L"..\\bin\\"
#define PATH_MUXER "..\\bin\\"
#define PATH_DECODER "..\\bin\\"
#define CREATE_PROCESS_FLAGS (CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED)
#else
#define PATH_SPLITTER
#define PATH_MUXER
#define PATH_DECODER
#define CREATE_PROCESS_FLAGS (CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP)
#endif


extern string program_path;

#define FILTER_NAME "ssifSource"
#define FRAME_START -1
#define FRAME_BLACK -2

std::string IntToStr(int a) {
	char buffer[32];
	_itoa_s(a, buffer, 32, 10);
	return buffer;
}

PClip ClipStack(IScriptEnvironment* env, PClip a, PClip b, bool horizontal) {
	const char* arg_names[2] = {NULL, NULL};
	AVSValue args[2] = {a, b};
	return (env->Invoke(horizontal ? "StackHorizontal" : "StackVertical", AVSValue(args,2), arg_names)).AsClip();
}

PVideoFrame FrameStack(IScriptEnvironment* env, VideoInfo& vi, PVideoFrame a, PVideoFrame b, bool horizontal) {
	return ClipStack(env, new FrameHolder(vi, a), new FrameHolder(vi, b), horizontal)->GetFrame(0, env);
}




string SSIFSource::MakePipeName(int id, const string& name) {
	return (string)"\\\\.\\pipe\\bluray" + IntToStr(id) + "\\" + name;
}

HRESULT CreateDumpFilter(const IID& riid, LPVOID *pFilter) {
//  for debugging with installed filter
//	return CoCreateInstance(CLSID_DumpFilter, NULL, CLSCTX_INPROC_SERVER, riid, pFilter);

	HRESULT hr = S_OK;
	CUnknown *filter = CDump::CreateInstance(NULL, &hr);
	if (SUCCEEDED(hr))
		return filter->NonDelegatingQueryInterface(riid, pFilter);
	return hr;
}

HRESULT SSIFSource::CreateGraph(const WCHAR* fnSource, const WCHAR* fnBase, const WCHAR* fnDept,
					CComPtr<IGraphBuilder>& poGraph, CComPtr<IBaseFilter>& poSplitter)
{
	HRESULT hr = S_OK;
	IGraphBuilder *pGraph = NULL;
	CComQIPtr<IBaseFilter> pSplitter;
	CComQIPtr<IBaseFilter> pDumper1, pDumper2;
	LPOLESTR lib_Splitter = T2OLE(PATH_SPLITTER L"MpegSplitter_mod.ax");
	const CLSID *clsid_Splitter = &CLSID_MpegSplitter;

	WCHAR fullname_splitter[MAX_PATH+64];
	size_t len;
	if (GetModuleFileNameW(hInstance, fullname_splitter, MAX_PATH) == 0)
		return E_UNEXPECTED;
	StringCchLengthW(fullname_splitter, MAX_PATH, &len);
	while (len > 0 && fullname_splitter[len-1] != '\\') --len;
	fullname_splitter[len] = '\0';
	StringCchCatW(fullname_splitter, MAX_PATH+64, lib_Splitter);

	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
	if (FAILED(hr)) goto lerror;
	hr = DSHelpCreateInstance(fullname_splitter, *clsid_Splitter, NULL, IID_IBaseFilter, (void**)&pSplitter);
	if (FAILED(hr)) goto lerror;
	hr = CreateDumpFilter(IID_IBaseFilter, (void**)&pDumper1);
	if (FAILED(hr)) goto lerror;
	if (fnDept) {
		hr = CreateDumpFilter(IID_IBaseFilter, (void**)&pDumper2);
		if (FAILED(hr)) goto lerror;
	}

	hr = pGraph->AddFilter(pSplitter, L"VSplitter");
	if (FAILED(hr)) goto lerror;
	hr = CComQIPtr<IFileSourceFilter>(pSplitter)->Load(fnSource, NULL);
	if (FAILED(hr)) goto lerror;

	hr = pGraph->AddFilter(pDumper1, L"VDumper1");
	if (FAILED(hr)) goto lerror;
	hr = CComQIPtr<IFileSinkFilter>(pDumper1)->SetFileName(fnBase, NULL);
	if (FAILED(hr)) goto lerror;

	if (pDumper2) {
		hr = pGraph->AddFilter(pDumper2, L"VDumper2");
		if (FAILED(hr)) goto lerror;
		hr = CComQIPtr<IFileSinkFilter>(pDumper2)->SetFileName(fnDept, NULL);
		if (FAILED(hr)) goto lerror;
	}

	hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 0, true), GetInPin(pDumper1, 0), NULL);
	if (FAILED(hr)) goto lerror;
	if (pDumper2) {
		hr = pGraph->ConnectDirect(GetOutPin(pSplitter, 1, true), GetInPin(pDumper2, 0), NULL);
		if (FAILED(hr)) goto lerror;
	}

	poGraph = pGraph;
	poSplitter = pSplitter;
	return S_OK;
lerror:
	return E_FAIL;
}

void SSIFSource::ParseEvents() {
	if (!pGraph)
		return;

	CComQIPtr<IMediaEventEx> pEvent = pGraph;
	long evCode = 0;
	LONG_PTR param1 = 0, param2 = 0;
	HRESULT hr = S_OK;
	while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
		// Invoke the callback.
		if (evCode == EC_COMPLETE) {
			CComQIPtr<IMediaControl>(pGraph)->Stop();
			pSplitter = NULL;
			pGraph = NULL;
		}

		// Free the event data.
		hr = pEvent->FreeEventParams(evCode, param1, param2);
		if (FAILED(hr))
			break;
	}
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
	
	last_frame = FRAME_BLACK;

#ifdef _DEBUG
	AllocConsole();
#endif
	memset(&SI, 0, sizeof(STARTUPINFO));
	SI.cb = sizeof(SI);
	SI.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
	if (data.flag_debug)
		SI.wShowWindow = SW_SHOWNORMAL;
	else
		SI.wShowWindow = SW_HIDE;

	memset(&PI1, 0, sizeof(PROCESS_INFORMATION));
	PI1.hProcess = INVALID_HANDLE_VALUE;
	memset(&PI2, 0, sizeof(PROCESS_INFORMATION));
	PI2.hProcess = INVALID_HANDLE_VALUE;

	frLeft = frRight = NULL;
	dupThread1 = dupThread2 = dupThread3 = NULL;
	unic_number = rand();

	pipes_over_warning = false;
}

void SSIFSource::InitDemuxer() {
	USES_CONVERSION;
	string
		fiBase = MakePipeName(unic_number, "base.h264"),
		fiDept = MakePipeName(unic_number, "dept.h264"),
		foBase = MakePipeName(unic_number, "base_merge.h264"),
		foDept = MakePipeName(unic_number, "dept_merge.h264");

	if (data.stop_after == SA_DEMUXER) {
		fiBase = data.left_264;
		fiDept = data.right_264;
	}
	else {
		if (data.left_264 == "")
			dupThread2 = new PipeDupThread(fiBase.c_str(), foBase.c_str());
		else
			dupThread2 = new PipeCloneThread(fiBase.c_str(), foBase.c_str(), data.left_264.c_str());
		data.left_264 = foBase;

		if (data.show_params & SP_RIGHTVIEW) {
			if (data.right_264 == "")
				dupThread3 = new PipeDupThread(fiDept.c_str(), foDept.c_str());
			else
				dupThread3 = new PipeCloneThread(fiDept.c_str(), foDept.c_str(), data.right_264.c_str());
		}
		data.right_264 = foDept;
	}

	CoInitialize(NULL);
	HRESULT res = CreateGraph(
		A2W(data.ssif_file.c_str()),
		A2W(fiBase.c_str()),
		(data.show_params & SP_RIGHTVIEW) ? A2W(fiDept.c_str()): NULL,
		pGraph, pSplitter);
	if (FAILED(res))
		throw (string)"Error creating graph. Code: " + IntToStr(res);

	res = CComQIPtr<IMediaControl>(pGraph)->Run();
	if (FAILED(res))
		throw (string)"Can't start the graph";
	ParseEvents();

	// Retrieve width & height
	CComPtr<IPin> pPin = GetOutPin(pSplitter, 0);
	CComPtr<IEnumMediaTypes> pEnum;
	AM_MEDIA_TYPE *pmt = NULL;
	res = pPin->EnumMediaTypes((IEnumMediaTypes**)&pEnum);
	if (SUCCEEDED(res)) {
		while (res = pEnum->Next(1, &pmt, NULL), res == S_OK) {
			if (pmt->majortype == MEDIATYPE_Video) {
				if (pmt->formattype == FORMAT_MPEG2_VIDEO) {
					// Found a match
					int *vih = (int*)pmt->pbFormat;
					frame_vi.width = data.dim_width = vih[19];
					frame_vi.height = data.dim_height = vih[20];
					DeleteMediaType(pmt);
					break;
				}
			}
			DeleteMediaType(pmt);
		}
		pEnum = NULL;
		pPin = NULL;
	}
}

void SSIFSource::InitMuxer() {
	if (!(data.show_params & SP_RIGHTVIEW)) {
		data.h264muxed = data.left_264;
		return;
	}

	string
		fiMuxed = MakePipeName(unic_number, "muxed.h264"),
		foMuxed = MakePipeName(unic_number, "intel_input.h264");

	if (data.stop_after == SA_MUXER) {
		fiMuxed = data.h264muxed;
	}
	else {
		if (data.h264muxed == "")
			dupThread1 = new PipeDupThread(fiMuxed.c_str(), foMuxed.c_str());
		else
			dupThread1 = new PipeCloneThread(fiMuxed.c_str(), foMuxed.c_str(), data.h264muxed.c_str());
		data.h264muxed = foMuxed;
	}

	string
		name_muxer = program_path + PATH_MUXER "mvccombine.exe",
		cmd_muxer = "\"" + name_muxer + "\" "
			"-l \"" + data.left_264 + "\" " +
			"-r \"" + data.right_264 + "\" " +
			"-o \"" + fiMuxed + "\" ";

	if (!CreateProcessA(name_muxer.c_str(), const_cast<char*>(cmd_muxer.c_str()), NULL, NULL, false, 
			CREATE_PROCESS_FLAGS, NULL, NULL, &SI, &PI1))
	{
		throw (string)"Error while launching " + name_muxer + "\n";
	}
}

void SSIFSource::InitDecoder() {
	bool flag_mvc = (data.show_params & SP_RIGHTVIEW) != 0;
	string name_decoder, s_dec_left_write, s_dec_right_write, s_dec_out, cmd_decoder;

	if (data.flag_use_ldecod) {
		name_decoder = program_path + PATH_DECODER "ldecod.exe";
		s_dec_left_write = MakePipeName(unic_number, "1_ViewId0000.yuv");
		s_dec_right_write = MakePipeName(unic_number, "1_ViewId0001.yuv");
		s_dec_out = MakePipeName(unic_number, "1.yuv");

		cmd_decoder = "\"" + name_decoder + "\" -p InputFile=\"" + data.h264muxed + "\" "
			"-p OutputFile=\"" + s_dec_out + "\" "
			"-p WriteUV=1 -p FileFormat=0 -p RefOffset=0 -p POCScale=2"
			"-p DisplayDecParams=1 -p ConcealMode=0 -p RefPOCGap=2 -p POCGap=2 -p Silent=0 -p IntraProfileDeblocking=1 -p DecFrmNum=0 "
			"-p DecodeAllLayers=1";
		flag_mvc = true;
	}
	else {
		name_decoder = program_path + PATH_DECODER "sample_decode.exe",
		s_dec_left_write = MakePipeName(unic_number, "output_0.yuv"),
		s_dec_right_write = MakePipeName(unic_number, "output_1.yuv"),
		s_dec_out = MakePipeName(unic_number, "output"),
		cmd_decoder = "\"" + name_decoder + "\" ";
		if (!(!data.inteldecoder_params.empty() && data.inteldecoder_params[0] != '-'))
			cmd_decoder += (flag_mvc ? "mvc" : "h264");
		cmd_decoder += " " + data.inteldecoder_params +	" -i \"" + data.h264muxed + "\" -o " + s_dec_out;
	}

	int framesize = frame_vi.width*frame_vi.height + (frame_vi.width*frame_vi.height/2)&~1;
	if (!flag_mvc)
		s_dec_left_write = s_dec_out;

	frLeft = new FrameSeparator(s_dec_left_write.c_str(), framesize);
	if (flag_mvc)
		frRight = new FrameSeparator(s_dec_right_write.c_str(), framesize);

	if (!CreateProcessA(name_decoder.c_str(), const_cast<char*>(cmd_decoder.c_str()), NULL, NULL, false,
			CREATE_PROCESS_FLAGS, NULL, NULL, &SI, &PI2))
	{
		throw (string)"Error while launching " + name_decoder + "\n";
	}
}

void SSIFSource::InitComplete() {
	vi = frame_vi;
	if ((data.show_params & (SP_LEFTVIEW | SP_RIGHTVIEW)) == (SP_LEFTVIEW | SP_RIGHTVIEW))
		((data.show_params & SP_HORIZONTAL) ? vi.width : vi.height) *= 2;

	ResumeThread(PI1.hThread);
	ResumeThread(PI2.hThread);
	if (data.stop_after == SA_DECODER)
		last_frame = FRAME_START;
}

SSIFSource* SSIFSource::Create(IScriptEnvironment* env, const SSIFSourceParams& data) {
	SSIFSource *res = new SSIFSource();
	res->data = data;
	SSIFSourceParams *cdata = &res->data;
	res->InitVariables();

	try {
		if (cdata->ssif_file != "")
			res->InitDemuxer();
		if (cdata->stop_after != SA_DEMUXER) {
			if (cdata->left_264 != "" && cdata->right_264 != "")
				res->InitMuxer();
			if (cdata->stop_after != SA_MUXER) {
				if (cdata->h264muxed != "")
					res->InitDecoder();
				else
					throw (string)"Can't decode nothing";
			}
		}
		res->InitComplete();
	}
	catch(const string& obj) {
		delete res;
		env->ThrowError((FILTER_NAME ": " + obj).c_str());
	}
	return res;
}

SSIFSource::~SSIFSource() {
	if (PI2.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI2.hProcess, 0);
	if (PI1.hProcess != INVALID_HANDLE_VALUE)
		TerminateProcess(PI1.hProcess, 0);
	if (dupThread1 != NULL)
		delete dupThread1;
	if (dupThread2 != NULL)
		delete dupThread2;
	if (dupThread3 != NULL)
		delete dupThread3;
	if (frLeft != NULL) 
		delete frLeft;
	if (frRight != NULL)
		delete frRight;
	pSplitter = NULL;
	pGraph = NULL;
	CoUninitialize();
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
	if (frSep != NULL) {
		if (!frSep->error)
	        frSep->WaitForData();
		frSep->DataParsed();
	}
}

PVideoFrame WINAPI SSIFSource::GetFrame(int n, IScriptEnvironment* env) {
	ParseEvents();

	if (last_frame+1 != n || n >= vi.num_frames) {
		string str;
		if ((last_frame == FRAME_BLACK) && (data.stop_after != SA_DECODER)) {
			if (pGraph != NULL) {
				CComQIPtr<IMediaSeeking> pSeeking = pGraph;
				REFERENCE_TIME tmDuration, tmPosition;
				pSeeking->GetDuration(&tmDuration);
				pSeeking->GetCurrentPosition(&tmPosition);
				int value = (double)tmPosition / tmDuration * 100.0;
				str = "Demuxing process: " + IntToStr(value) + "% completed.";
			}
			else {
				str = "Demuxing process completed.";
			}
		}
		else {
			str = "ERROR:\\n"
				  "Can't retrieve frame #" + IntToStr(n) + " !\\n";
			if (last_frame >= vi.num_frames-1)
				str += "Video sequence is over. Reload the script.\\n";
			else
				str += "Frame #" + IntToStr(last_frame+1) + " should be the next frame.\\n";
			str += "Note: " FILTER_NAME " filter supports only sequential frame rendering.";
		}
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

	data.ssif_file = args[0].AsString("");
	data.left_264 = args[8].AsString("");
	data.right_264 = args[9].AsString("");
	data.h264muxed = args[10].AsString("");
	data.frame_count = args[1].AsInt();
	data.dim_width = args[11].AsInt(1920);
	data.dim_height = args[12].AsInt(1080);
	data.show_params = 
		(args[2].AsBool(true) ? SP_LEFTVIEW : 0) |
		(args[3].AsBool(true) ? SP_RIGHTVIEW : 0) |
		(args[4].AsBool(false) ? SP_HORIZONTAL : 0) |
		(args[5].AsBool(false) ? SP_SWAPVIEWS : 0);
	data.inteldecoder_params = args[6].AsString("");
	data.flag_debug = args[7].AsBool(false);
	if (!(data.show_params & (SP_LEFTVIEW | SP_RIGHTVIEW))) {
        env->ThrowError(FILTER_NAME ": can't show nothing");
    }
	data.stop_after = args[13].AsInt(SA_DECODER);
	data.flag_use_ldecod = args[14].AsBool(false);
	return SSIFSource::Create(env, data);
}
