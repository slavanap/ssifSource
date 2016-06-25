// 2D subtitle extraction AviSynth plugin
// Copyright 2016 Vyacheslav Napadovsky.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Filter.RestoreAlpha.hpp"

#include "common.h"

using namespace Tools::AviSynth;

namespace Filter {

	static inline void RestoreAlpha(int o1, int bk1, int o2, int bk2, BYTE& x, BYTE& a) {
		assert(o1 != bk1 // 1st background color should change after overlay
			&& o2 != bk2 // 2nd background color should change after overlay
			&& bk1 != bk2 // background colors must differ
		);
		// lazy version
		float ra = 1.0f - ((float)(o2 - o1)) / (bk2 - bk1);
		int rx = (int)((o1 - bk1) / ra) + bk1;
		x = Frame::Pixel::ColorLimits(rx);
		a = Frame::Pixel::ColorLimits((int)(ra * 255));
		// optimized version:
		//a = Frame::Pixel::ColorLimits(255 - (o2 - o1) * 255 / (bk2 - bk1));
		//x = Frame::Pixel::ColorLimits((o1 - bk1) * (bk2 - bk1) / (bk2 - bk1 + o1 - o2));
	}

	static inline void RestoreChannel(BYTE o1, BYTE bk1, BYTE o2, BYTE bk2, BYTE o3, BYTE bk3, BYTE& x, BYTE& a) {
		if (o1 == bk1 && o2 == bk2 && o3 == bk3) {
			x = 0, a = 0;
			return;
		}
		if ((o1 == bk1 && (o2 == bk2 || o3 == bk3)) || (o2 == bk2 && o3 == bk3)) {
			assert(false && "precision assert");
			x = 0, a = 0;
			return;
		}

		if (o1 == bk1) {
			// assume o2 != bk2 && o3 != bk3
			RestoreAlpha(o2, bk2, o3, bk3, x, a);
		}
		else if (o2 == bk2) {
			// assume o1 != bk1 && o3 != bk3
			RestoreAlpha(o1, bk1, o3, bk3, x, a);
		}
		else {
			// assume o1 != bk1 && o2 != bk2 (see conditions above)
			RestoreAlpha(o1, bk1, o2, bk2, x, a);
		}
	}

	static inline Frame::Pixel RestoreColor(
		const Frame::Pixel& o1, const Frame::Pixel& bk1,
		const Frame::Pixel& o2, const Frame::Pixel& bk2,
		const Frame::Pixel& o3, const Frame::Pixel& bk3)
	{
		Frame::Pixel res;
		BYTE alpha[3];
		RestoreChannel(o1.r, bk1.r, o2.r, bk2.r, o3.r, bk3.r, res.r, alpha[0]);
		RestoreChannel(o1.g, bk1.g, o2.g, bk2.g, o3.g, bk3.g, res.g, alpha[1]);
		RestoreChannel(o1.b, bk1.b, o2.b, bk2.b, o3.b, bk3.b, res.b, alpha[2]);
		res.a = std::max(std::max(alpha[0], alpha[1]), alpha[2]);
		return res;
	}




	RestoreAlpha::RestoreAlpha(IScriptEnvironment* env, const std::string& funcname, PClip reference) :
		GenericVideoFilter(reference)
	{
		int colors[SAMPLES] = { 0x404040, 0x808080, 0xC0C0C0 };
		for (int i = 0; i < ARRAY_SIZE(colors); ++i) {
			bkcolor[i].toUInt() = (uint32_t)colors[i];
			clipOverlay[i] = AvsCall(env, funcname.c_str(),
				AvsCall(env, "BlankClip", reference, AvsNamedArg("color", colors[i]))).AsClip();
			CheckVideoInfo(env, vi, clipOverlay[i]->GetVideoInfo());
		}
	}

	PVideoFrame __stdcall RestoreAlpha::GetFrame(int n, IScriptEnvironment* env) {
		Frame fsrc[SAMPLES];
		for (int i = 0; i < SAMPLES; ++i)
			fsrc[i] = Frame(env, clipOverlay[i], n);
		Frame fres(env, vi);
#pragma omp parallel for schedule(guided)
		for (int y = 0; y < fres.height(); ++y) {
			for (int x = 0; x < fres.width(); ++x) {
				Frame::Pixel &pres = fres.write(x, y);
				const Frame::Pixel pout[SAMPLES] = { fsrc[0].read(x, y), fsrc[1].read(x, y), fsrc[2].read(x, y) };
				if (pout[0] == bkcolor[0] && pout[1] == bkcolor[1] && pout[2] == bkcolor[2]) {
					pres.toUInt() = 0;
				}
				else if (pout[0] == pout[1] && pout[0] == pout[2]) {
					pres = pout[0];
				}
				else {
					pres = RestoreColor(pout[0], bkcolor[0], pout[1], bkcolor[1], pout[2], bkcolor[2]);
				}
			}
		}
		return fres;
	}

	
	
	AvsParams RestoreAlpha::CreateParams = "[renderer]s[reference]c";
	
	AVSValue __cdecl RestoreAlpha::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return new RestoreAlpha(env, args[0].AsString(), args[1].AsClip());
	}

}
