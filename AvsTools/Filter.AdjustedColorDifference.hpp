//
// Adjusted Color Difference filter
// Copyright 2017 Vyacheslav Napadovsky.
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


#pragma once

#include "Tools.AviSynth.Frame.hpp"

namespace Filter {

	class AdjustedColorDifference : public GenericVideoFilter {
	public:
		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
	private:
		double m_factor;
		PClip m_subtrahend;
		AdjustedColorDifference(IScriptEnvironment* env, PClip input, double factor, PClip subtrahend = PClip());
	};

}
