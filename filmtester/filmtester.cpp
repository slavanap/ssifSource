#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avisynth.h>
#include <string>
#include <vector>

using namespace std;

#define SPACES "                          "

PClip clip;
PVideoFrame dst, prev_frame;

CRITICAL_SECTION cs;

BOOL CtrlHandler(DWORD fdwCtrlType) {
	switch(fdwCtrlType)	{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
			EnterCriticalSection(&cs);
			dst = NULL;
			prev_frame = NULL;
			clip = NULL;
			return FALSE;

		default:
			return FALSE;
	}
}

int main(int argc, TCHAR* argv[]) {
	printf("Usage: filmtester <avs filename> [duplicates_maxlength=2]\n");
	printf("The program plays the AVS file and tests for frame duplicates\n\n");

	int duplicates_maxlength = 2;
	if (argc < 2) {
		printf("No filename specified.\n\n");
		return -1;
	}
	if (argc > 2) {
		duplicates_maxlength = atoi(argv[2]);
		printf("INFO: duplicates_maxlength set to %d\n", duplicates_maxlength);
	}

	IScriptEnvironment *env = CreateScriptEnvironment();
	printf("Loading \"%s\" ...\n", argv[1]);

	const char* arg_names[1] = {NULL};
	AVSValue arg_vals[1] = {(LPCSTR)argv[1]};
	clip = (env->Invoke("import", AVSValue(arg_vals,1), arg_names)).AsClip();

	printf("AVS file loaded successfully.\n\n");

	VideoInfo vi = clip->GetVideoInfo();
	printf("VideoInfo:\n");
	printf("-----------\n");
	if (vi.HasVideo()) {
		printf("width x height: %dx%d\n", vi.width, vi.height);
		printf("num_frames: %d\n", vi.num_frames);
		printf("fps: %d/%d\n", vi.fps_numerator, vi.fps_denominator);

		string colorspace = "";
		if (vi.pixel_type & VideoInfo::CS_BGR) colorspace += "BGR, ";
		if (vi.pixel_type & VideoInfo::CS_YUV) colorspace += "YUV, ";
		if (vi.pixel_type & VideoInfo::CS_INTERLEAVED) colorspace += "INTERLEAVED, ";
		if (vi.pixel_type & VideoInfo::CS_PLANAR) colorspace += "PLANAR, ";
		if (colorspace.length() > 0) colorspace.erase(colorspace.length()-2);
		printf("colorspace: %s\n", colorspace.c_str());

		string colorformat = "";
		if (vi.pixel_type & VideoInfo::CS_BGR24) colorformat += "BGR24, ";
		if (vi.pixel_type & VideoInfo::CS_BGR32) colorformat += "BGR32, ";
		if (vi.pixel_type & VideoInfo::CS_YUY2)  colorformat += "YUY2, ";
		if (vi.pixel_type & VideoInfo::CS_YV12)  colorformat += "YV12, ";
		if (vi.pixel_type & VideoInfo::CS_I420)  colorformat += "I420 (IYUV), ";
		if (colorformat.length() > 0)
			colorformat.erase(colorformat.length()-2);
		else
			colorformat = "UNKNOWN";
		printf("colorformat: %s\n", colorformat.c_str());

		string imagetype = "";
		if (vi.image_type & VideoInfo::IT_BFF) imagetype += "BFF, ";
		if (vi.image_type & VideoInfo::IT_TFF) imagetype += "TFF, ";
		if (vi.image_type & VideoInfo::IT_FIELDBASED)  imagetype += "FIELDBASED, ";
		if (imagetype.length() > 0)
			imagetype.erase(imagetype.length()-2);
		else
			imagetype = "UNKNOWN";
		printf("image_type: %s\n", imagetype.c_str());
		printf("bits per pixel: %d\n", vi.BitsPerPixel());
	}
	else
		printf("NO VIDEO\n");

	if (vi.HasAudio()) {
		printf("audio channels: %d\n", vi.nchannels);
		printf("sample_type: %x\n", vi.sample_type);
		printf("samples per second: %d\n", vi.audio_samples_per_second);
		printf("bytes per channel sample: %d\n", vi.BytesPerChannelSample());
		printf("bytes per audio sample: %d\n", vi.BytesPerAudioSample());
		printf("num_audio_samples: %d\n", vi.num_audio_samples);
	}
	else
		printf("NO AUDIO\n");
	printf("-----------\n\n");

	if (!vi.HasVideo()) {
		printf("Can't start video playback for the sequence without video.\n\n");
		return -1;
	}

	printf("Starting playback ...\n");
	prev_frame = clip->GetFrame(0, env);
	int framesize = prev_frame->GetFrameBuffer()->GetDataSize();
	printf("INFO: framesize = %d bytes.\n\n", framesize);

	InitializeCriticalSection(&cs);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	int error_count = 0;
	int dup_start_frame = 0;
	bool flag_dup = false;
	vector<pair<int,int> > duplicates;
	for(int i=1; i<vi.num_frames; ++i) {
		EnterCriticalSection(&cs);
		dst = clip->GetFrame(i, env);
		const BYTE *src_ptr = prev_frame->GetFrameBuffer()->GetReadPtr();
		const BYTE *dst_ptr = dst->GetFrameBuffer()->GetReadPtr();
		if (!memcmp(src_ptr, dst_ptr, framesize)) {
			if (!flag_dup) {
				flag_dup = true;
				dup_start_frame = i-1;
			}
		}
		else if (flag_dup) {
			int length = (i-1) - dup_start_frame;
			if (length >= duplicates_maxlength) {
				printf("\rfilmtester: duplication interval: %d..%d" SPACES "\n", dup_start_frame, i-1);
				duplicates.push_back(make_pair(dup_start_frame, i-1));
				error_count++;
			}
			flag_dup = false;
		}
		prev_frame = dst;
		LeaveCriticalSection(&cs);
		printf("\r[%5.1f%%] [%d errors] %d/%d frame processing", (float)((i+1)*100)/vi.num_frames, error_count, i+1, vi.num_frames);
	}

	EnterCriticalSection(&cs);
	if (flag_dup) {
		int i = vi.num_frames;
		int length = (i-1) - dup_start_frame;
		if (length >= duplicates_maxlength) {
			printf("\rfilmtester: duplication interval: %d..%d" SPACES "\n", dup_start_frame, i-1);
			duplicates.push_back(make_pair(dup_start_frame, i-1));
			error_count++;
		}
		flag_dup = false;
	}
	printf("\rProcessing completed." SPACES "\n\n");
	printf("%d errors\n", error_count);
	printf("\n");
	if (error_count > 0) {
		printf("Erroneous intervals (%d):\n", duplicates.size());
		for(vector<pair<int,int> >::const_iterator it = duplicates.begin(); it != duplicates.end(); ++it)
			printf("%5d .. %d\n", it->first, it->second);
		printf("\n");
	}
	dst = NULL;
	prev_frame = NULL;
	clip = NULL;

	LeaveCriticalSection(&cs);
	DeleteCriticalSection(&cs);
	return 0;
}
