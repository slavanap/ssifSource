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

#include "stdafx.h"
#include <Filter.CropDetect.hpp>
#include <Filter.Pipe.hpp>
#include <Tools.WinApi.hpp>

#include "Filter.mplsSource.hpp"
#include "Filter.mplsSource2.hpp"
#include "Filter.ssifSource.hpp"

// DirectShow initialization
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
			return Tools::WinApi::InitLibrary(hModule);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

// AviSynth functions registration
extern "C" __declspec(dllexport) 
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("CreateReadPipe", Filter::Pipe::CreateReadPipeParams, Filter::Pipe::CreateReadPipe, nullptr);
	env->AddFunction("CreateWritePipe", Filter::Pipe::CreateWritePipeParams, Filter::Pipe::CreateWritePipe, nullptr);
	env->AddFunction("PipeReader", Filter::Pipe::PipeReader::CreateParams, Filter::Pipe::PipeReader::Create, nullptr);
	env->AddFunction("PipeReaderForHandle", Filter::Pipe::PipeReader::CreateForHandleParams, Filter::Pipe::PipeReader::CreateForHandle, nullptr);
	env->AddFunction("PipeWriter", Filter::Pipe::PipeWriter::CreateParams, Filter::Pipe::PipeWriter::Create, nullptr);
	env->AddFunction("PipeWriterForHandle", Filter::Pipe::PipeWriter::CreateForHandleParams, Filter::Pipe::PipeWriter::CreateForHandle, nullptr);
	env->AddFunction("CropDetect", Filter::CropDetect::CreateParams, Filter::CropDetect::Create, nullptr);
	env->AddFunction("ssifSource", Filter::ssifSource::CreateParams, Filter::ssifSource::Create, nullptr);
	env->AddFunction("mplsSource", Filter::mplsSource::CreateParams, Filter::mplsSource::Create, nullptr);
	env->AddFunction("mplsSource2", Filter::mplsSource2::CreateParams, Filter::mplsSource2::Create, nullptr);
	return nullptr;
}
