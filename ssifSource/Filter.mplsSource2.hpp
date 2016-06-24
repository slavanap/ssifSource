// BD3D video decoding tool
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

#pragma once
#include "Tools.AviSynth.hpp"
#include "Tools.Pipe.hpp"
#include "Tools.WinApi.hpp"

namespace Filter {

	using namespace Tools::Pipe;
	using namespace Tools::WinApi;

	class mplsSource2 : public Tools::AviSynth::SourceFilterStub {
	public:
		mplsSource2(IScriptEnvironment* env, AVSValue args);

		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

	private:
		PClip currentClip;
		std::string ssifPath;
		bool flagMVC;
		bool flagSwapViews;
		std::unique_ptr<ProxyThread> proxyLeft, proxyRight;
		std::unique_ptr<ProcessHolder> process;
		UniqueIdHolder uniqueId;
	};


}
