#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

#include <avisynth.h>

const int threshold = 40 - 16;
const int detect_frames = 20;

class CropDetect: public IClip {
private:
	PClip left, right;
	FILE *outfile;
	VideoInfo vi;
	CropDetect(PClip left, PClip right, FILE* outfile, IScriptEnvironment* env);
	CropDetect(const CropDetect& obj) {}
protected:
	void DetectForFrame(PVideoFrame vf, RECT* res);
public:
	static AVSValue Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	virtual ~CropDetect();

	// AviSynth virtual functions
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env);

	bool WINAPI GetParity(int n) { return false; }
	void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
	const VideoInfo& WINAPI GetVideoInfo() { return vi; }
	void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};

PClip CheckForRGB32(IScriptEnvironment* env, PClip src) {
	if (src->GetVideoInfo().IsRGB32())
		return src;
	const char* arg_names[1] = {NULL};
	AVSValue args[1] = {src};
	return (env->Invoke("ConvertToRGB32", AVSValue(args,1), arg_names)).AsClip();
}

void CropDetect::DetectForFrame(PVideoFrame vf, RECT* res) {
	const BYTE *data = vf->GetReadPtr();

	int found;

	// Find the top
	found = 0;
	for (int ctop = 0; ctop < vi.height-1; ctop++) {
		const BYTE *row = data + ((vi.height-1)-ctop) * vf->GetPitch();	// flip
		int lum = 0;
		// Sum the luminance for that line
		for (int x = 0; x < vi.width; x++, row+=4)
			lum += (257 * row[2]) + (504 * row[1]) + (98 * row[0]);
		// If the average luminance if > threshold we've found the top
		if ((lum/1000) / vi.width >= threshold) {
			found++;
			if (found==3) {
				if (ctop-2 < res->top)
					res->top = ctop-2;
				break;
			}
		} else
			found = 0;
	}

	// Find the bottom
	found = 0;
	for (int cbottom = vi.height-1; cbottom > 0; cbottom--) {
		const BYTE *row = data + ((vi.height-1)-cbottom) * vf->GetPitch();	// flip
		int lum = 0;
		// Sum the luminance for that line
		for (int x = 0; x < vi.width; x++, row+=4)
			lum += (257 * row[2]) + (504 * row[1]) + (98 * row[0]);
		// If the average luminance if > threshold we've found the top
		if ((lum/1000) / vi.width >= threshold) {
			found++;
			if (found==3) {
				if (cbottom+2 > res->bottom)
					res->bottom = cbottom+2;							
				break;
			}
		} else
			found = 0;
	}

	// Find the left
	found = 0;
	for (int cleft = 0; cleft < vi.width-1; cleft++) {
		int lum = 0;
		// Sum the luminance for that column
		for (int y = 0; y < vi.height; y++) {
			const BYTE *elem = data + y * vf->GetPitch() + cleft*4;
			lum += (257 * elem[2]) + (504 * elem[1]) + (98 * elem[0]);
		}
		// If the average luminance if > threshold we've found the top
		if ((lum/1000) / vi.height >= threshold) {
			found++;
			if (found==3) {
				if (cleft-2 < res->left)
					res->left = cleft-2;							
				break;
			}
		} else
			found = 0;
	}

	// Find the right
	found = 0;
	for (int cright = vi.width-1; cright > 0; cright--) {
		int lum = 0;
		// Sum the luminance for that column
		for (int y = 0; y < vi.height; y++) {
			const BYTE *elem = data + y * vf->GetPitch() + cright*4;
			lum += (257 * elem[2]) + (504 * elem[1]) + (98 * elem[0]);
		}
		// If the average luminance if > threshold we've found the top
		if ((lum/1000) / vi.height >= threshold) {
			found++;
			if (found==3) {
				if (cright+2 > res->right)
					res->right = cright+2;							
				break;
			}
		} else
			found = 0;
	}
}

AVSValue CropDetect::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
	PClip src_left = CheckForRGB32(env, args[0].AsClip());
	PClip src_right = CheckForRGB32(env, args[1].AsClip());

	VideoInfo vil = src_left->GetVideoInfo();
	VideoInfo vir = src_right->GetVideoInfo();
	if (vil.width != vir.width || vil.height != vir.height || vil.num_frames != vir.num_frames)
		env->ThrowError("Resolution or num_frames mismatch");

	FILE *f = fopen(args[2].AsString(), "w");
	if (!f)
		env->ThrowError("Error to open output file");

	return new CropDetect(src_left, src_right, f, env);
}

CropDetect::CropDetect(PClip left, PClip right, FILE* outfile, IScriptEnvironment* env)
: left(left), right(right), outfile(outfile)
{
	vi = this->left->GetVideoInfo();

	RECT rleft  = {MAXINT,MAXINT,MININT,MININT};
	RECT rright = {MAXINT,MAXINT,MININT,MININT};
	for(int idx=0; idx<detect_frames; ++idx) {
		int n = (vi.num_frames-1) * idx / (detect_frames-1);
		PVideoFrame frame_left = left->GetFrame(n, env);
		PVideoFrame frame_right = right->GetFrame(n, env);
		DetectForFrame(frame_left, &rleft);
		DetectForFrame(frame_right, &rright);
	}

	RECT res;
	res.left   = min( rleft.left,   rright.left   );
	res.top    = min( rleft.top,    rright.top    );
	res.right  = max( rleft.right,  rright.right  );
	res.bottom = max( rleft.bottom, rright.bottom );

	//fprintf(outfile, "Crop(%d,%d,%d,%d)\n", res.left, res.top, res.right-res.left+1, res.bottom-res.top+1);
	fprintf(outfile, "Crop(%d,%d,%d,%d)\n", res.left, res.top, res.right - (vi.width-1), res.bottom - (vi.height-1));
	fclose(outfile);
}

CropDetect::~CropDetect() {}

PVideoFrame CropDetect::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame frame_left = left->GetFrame(n, env);
	PVideoFrame frame_right = right->GetFrame(n, env);
	return frame_left;
}

extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("CropDetect", "[left]c[right]c[outfile]s", CropDetect::Create, 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			return TRUE;
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
