#include "stdafx.h"
#include "Filter.CropDetect.hpp"

using namespace Tools::AviSynth;

namespace Filter {

	CropDetect::CropDetect(IScriptEnvironment* env, PClip clip, const char *outfile, int threshold, int detectFrames) :
		GenericVideoFilter(clip), m_threshold(threshold)
	{
		FILE *f;
		if (fopen_s(&f, outfile, "w") || f == nullptr)
			env->ThrowError("Error to open output file");

		clip = ConvertToRGB32(env, clip);
		RECT rect = { MAXINT, MAXINT, MININT, MININT };
		for (int idx = 0; idx < detectFrames; ++idx) {
			int n = (int)((INT64)(vi.num_frames - 1) * idx / (detectFrames - 1));
			AdjustForFrame(rect, Frame(env, clip, n));
		}

		fprintf(f, "Crop(%d,%d,%d,%d)\n", rect.left, rect.top, rect.right - (vi.width - 1), rect.bottom - (vi.height - 1));
		fclose(f);
	}

	static inline int GetLuminance(const Frame::Pixel& pixel) {
		return (257 * (int)pixel.r) + (504 * (int)pixel.g) + (98 * (int)pixel.b);
	}

	void CropDetect::AdjustForFrame(RECT& rect, const Frame& frame) {
		enum {
			LINES_IN_A_ROW = 3,
		};

		// Find the top
		int found = 0;
		for (int y = 0; y < frame.height() - 1; ++y) {
			auto row = frame.read_row(y);
			int lum = 0;
			// Sum the luminance for that line
			for (int x = 0; x < frame.width(); ++x)
				lum += GetLuminance(*row++);
			// If the average luminance if > threshold we've found the top
			if (CheckThreshold(lum, frame.width())) {
				found++;
				if (found == LINES_IN_A_ROW) {
					int newtop = y - (LINES_IN_A_ROW - 1);
					if (newtop < rect.top)
						rect.top = newtop;
					break;
				}
			}
			else
				found = 0;
		}

		// Find the bottom
		found = 0;
		for (int y = frame.height() - 1; y > 0; --y) {
			auto row = frame.read_row(y);
			int lum = 0;
			for (int x = 0; x < frame.width(); ++x)
				lum += GetLuminance(*row++);
			if (CheckThreshold(lum, frame.width())) {
				found++;
				if (found == LINES_IN_A_ROW) {
					int newbottom = y + (LINES_IN_A_ROW - 1);
					if (newbottom > rect.bottom)
						rect.bottom = newbottom;
					break;
				}
			}
			else
				found = 0;
		}

		// Find the left
		found = 0;
		for (int x = 0; x < frame.width() - 1; ++x) {
			int lum = 0;
			for (int y = 0; y < frame.height(); ++y)
				lum += GetLuminance(frame.read(x, y));
			if (CheckThreshold(lum, frame.height())) {
				found++;
				if (found == LINES_IN_A_ROW) {
					int newleft = x - (LINES_IN_A_ROW - 1);
					if (newleft < rect.left)
						rect.left = newleft;
					break;
				}
			}
			else
				found = 0;
		}

		// Find the right
		found = 0;
		for (int x = frame.width() - 1; x > 0; --x) {
			int lum = 0;
			for (int y = 0; y < frame.height(); ++y)
				lum += GetLuminance(frame.read(x, y));
			if (CheckThreshold(lum, frame.height())) {
				found++;
				if (found == LINES_IN_A_ROW) {
					int newright = x + (LINES_IN_A_ROW - 1);
					if (newright > rect.right)
						rect.right = newright;
					break;
				}
			}
			else
				found = 0;
		}
	}


	AvsParams CropDetect::CreateParams =
		"[src]c"
		"[outfile]s"
		"[threshold]i"
		"[detect_frames]i";

	AVSValue CropDetect::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		PClip temp_clip = new CropDetect(env, args[0].AsClip(), args[1].AsString(), args[2].AsInt(24), args[3].AsInt(20));
		return AVSValue();
	}

}
