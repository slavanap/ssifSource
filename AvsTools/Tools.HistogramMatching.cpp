//
// Histogram Matching algorithm
// Copyright 2016 Vyacheslav Napadovsky.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include "Tools.HistogramMatching.hpp"

namespace Tools {

	using AviSynth::Frame;

	// Histogram Matching Algorithm
	// Reference: http://en.wikipedia.org/wiki/Histogram_matching

	typedef int Histogram[Frame::ColorCount];

	static inline void HistogramToIntegralHistogram(Histogram& h) {
		for (int i = 1; i < Frame::ColorCount; ++i)
			h[i] += h[i - 1];
	}

	static inline void CalculateHistogramFunction(Histogram& input, Histogram& reference, Histogram& func) {
		HistogramToIntegralHistogram(reference);
		HistogramToIntegralHistogram(input);
		int il, ir;
		for (il = ir = 0; ir < Frame::ColorCount; ir++) {
			int value;
			for (value = input[ir]; (il < Frame::ColorCount - 1) && (reference[il] < value); ++il);
			if ((il > 0) && (reference[il] + reference[il - 1] > 2 * value))
				--il;
			func[ir] = il;
		}
	}

	void HistogramMatching(Frame& input, const Frame& reference) {

		assert(input.width() == reference.width() && input.height() == reference.height());

		// we going to modify input frame
		input.EnableWriting();

		Histogram ref_y, ref_r, ref_g, ref_b;
		Histogram inp_y;
#pragma omp sections  
		{
#pragma omp section  
			{
				// Calculate histogram for every color & brightness for REFERENCE frame
				memset(ref_y, 0, sizeof(Histogram));
				memset(ref_r, 0, sizeof(Histogram));
				memset(ref_g, 0, sizeof(Histogram));
				memset(ref_b, 0, sizeof(Histogram));
				for (auto it = reference.cbegin(); it != reference.cend(); ++it) {
					++ref_y[it->GetBrightness()];
					++ref_r[it->r];
					++ref_g[it->g];
					++ref_b[it->b];
				}
			}
#pragma omp section 
			{
				// Calculate histogram for brightness only for INPUT frame
				memset(inp_y, 0, sizeof(Histogram));
				for (auto it = input.cbegin(); it != input.cend(); ++it)
					++inp_y[it->GetBrightness()];
			}
		}

		// Calculate histogram function for brightness
		Histogram func_y;
		CalculateHistogramFunction(inp_y, ref_y, func_y);

		// Apply brightness function and calculate histogram for every color for INPUT frame
		Histogram inp_r, inp_g, inp_b;
		memset(inp_r, 0, sizeof(Histogram));
		memset(inp_g, 0, sizeof(Histogram));
		memset(inp_b, 0, sizeof(Histogram));
		for (auto it = input.begin(); it != input.end(); ++it) {
			int br = it->GetBrightness();
			int dy = func_y[br] - br;
			it->r = Frame::Pixel::ColorLimits(it->r + dy);
			++inp_r[it->r];
			it->g = Frame::Pixel::ColorLimits(it->g + dy);
			++inp_g[it->g];
			it->b = Frame::Pixel::ColorLimits(it->b + dy);
			++inp_b[it->b];
		}

		// Calculate histogram function for every color
		Histogram func_r, func_g, func_b;
		CalculateHistogramFunction(inp_r, ref_r, func_r);
		CalculateHistogramFunction(inp_g, ref_g, func_g);
		CalculateHistogramFunction(inp_b, ref_b, func_b);

		// Apply color transformation
#pragma omp parallel for
		for (int y = 0; y < input.height(); ++y) {
			Frame::Pixel *it = input.write_row(y);
			for (int x = 0; x < input.width(); ++x, ++it) {
				it->r = func_r[it->r],
				it->g = func_g[it->g],
				it->b = func_b[it->b];
			}

		}

	}

}

namespace Filter {

	using Tools::AviSynth::CheckVideoInfo;
	using Tools::AviSynth::Frame;

	HistogramMatching::HistogramMatching(IScriptEnvironment* env, PClip input, PClip reference) :
		GenericVideoFilter(input), m_reference(reference)
	{
		if (!vi.IsRGB32())
			env->ThrowError("plugin supports only RGB32 input");
		m_rvi = reference->GetVideoInfo();
		CheckVideoInfo(env, vi, m_rvi);
	}

	PVideoFrame HistogramMatching::GetFrame(int n, IScriptEnvironment* env) {
		Frame finp(env, child, n);
		Frame fref(env, m_reference, n);
		Tools::HistogramMatching(finp, fref);
		return finp;
	}


	AvsParams HistogramMatching::CreateParams =
		"[input]c"
		"[reference]c";
	
	AVSValue HistogramMatching::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		return new HistogramMatching(env, args[0].AsClip(), args[1].AsClip());
	}

}
