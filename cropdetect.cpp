#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <avisynth.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

class CropDetect : public GenericVideoFilter {
private:
	int m_threshold;
	CropDetect(IScriptEnvironment* env, PClip clip, const char *outfile, int threshold, int detectFrames);
protected:
	void DetectForFrame(PVideoFrame vf, RECT* res);
public:
	static AVSValue Create(AVSValue args, void* user_data, IScriptEnvironment* env);
};

extern "C" __declspec(dllexport)
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("CropDetect", "[src]c[outfile]s[threshold]i[detect_frames]i", CropDetect::Create, 0);
	return 0;
}

AVSValue CropDetect::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new CropDetect(env, args[0].AsClip(), args[1].AsString(), args[2].AsInt(24), args[3].AsInt(20));
}

PClip CheckForRGB32(IScriptEnvironment* env, PClip src) {
	if (src->GetVideoInfo().IsRGB32())
		return src;
	const char* arg_names[1] = { NULL };
	AVSValue args[1] = { src };
	return env->Invoke("ConvertToRGB32", AVSValue(args, 1), arg_names).AsClip();
}

CropDetect::CropDetect(IScriptEnvironment* env, PClip clip, const char *outfile, int threshold, int detectFrames)
	: GenericVideoFilter(CheckForRGB32(env, clip)), m_threshold(threshold)
{
	FILE *f;
	if (fopen_s(&f, outfile, "w") || f == nullptr)
		env->ThrowError("Error to open output file");

	RECT rect = { MAXINT, MAXINT, MININT, MININT };
	for (int idx = 0; idx < detectFrames; ++idx) {
		int n = (vi.num_frames - 1) * idx / (detectFrames - 1);
		DetectForFrame(clip->GetFrame(n, env), &rect);
	}

	fprintf(f, "Crop(%d,%d,%d,%d)\n", rect.left, rect.top, rect.right - (vi.width-1), rect.bottom - (vi.height-1));
	fclose(f);
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
		if ((lum/1000) / vi.width >= m_threshold) {
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
		// If the average luminance if > threshold we've found the bottom
		if ((lum/1000) / vi.width >= m_threshold) {
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
		// If the average luminance if > threshold we've found the left
		if ((lum/1000) / vi.height >= m_threshold) {
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
		// If the average luminance if > threshold we've found the right
		if ((lum/1000) / vi.height >= m_threshold) {
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
