//
// 3D subtitle rendering tool
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


#include "stdafx.h"
#include <Tools.WinApi.hpp>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			return Tools::WinApi::InitLibrary(hModule);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}



#include <Tools.AviSynth.hpp>
#include <Tools.HistogramMatching.hpp>
#include <Filter.CropDetect.hpp>
#include <Filter.Pipe.hpp>
#include <Filter.SequentialToSeekable.hpp>
#include <Filter.SetLogger.hpp>
#include "Filter.XMLRenderer.hpp"
#include "Filter.SRTRenderer.hpp"
#include "Filter.RestoreAlpha.hpp"
#include "Tools.Lua.hpp"

// AviSynth functions registration
extern "C" __declspec(dllexport)
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	// Sub3D
	env->AddFunction("CalcSRTDepths", Filter::SRTRenderer::Renderer::CalculateParams, Filter::SRTRenderer::Renderer::Calculate, nullptr);
	env->AddFunction("CalcXMLDepths", Filter::XMLRenderer::CalculateParams, Filter::XMLRenderer::Calculate, nullptr);
	env->AddFunction("RenderXML", Filter::XMLRenderer::RenderParams, Filter::XMLRenderer::Render, nullptr);
	env->AddFunction("RenderSRT", Filter::SRTRenderer::Renderer::RenderParams, Filter::SRTRenderer::Renderer::Render, nullptr);
	env->AddFunction("RestoreAlpha", Filter::RestoreAlpha::CreateParams, Filter::RestoreAlpha::Create, nullptr);
	env->AddFunction("SetDepthComputationAlg", Tools::Lua::SetLuaFileParams, Tools::Lua::SetLuaFile, nullptr);
	env->AddFunction("SetLogger", Filter::SetLoggerParams, Filter::SetLogger, nullptr);
	// Pipes
	env->AddFunction("CreateReadPipe", Filter::Pipe::CreateReadPipeParams, Filter::Pipe::CreateReadPipe, nullptr);
	env->AddFunction("CreateWritePipe", Filter::Pipe::CreateWritePipeParams, Filter::Pipe::CreateWritePipe, nullptr);
	env->AddFunction("PipeReader", Filter::Pipe::PipeReader::CreateParams, Filter::Pipe::PipeReader::Create, nullptr);
	env->AddFunction("PipeReaderForHandle", Filter::Pipe::PipeReader::CreateForHandleParams, Filter::Pipe::PipeReader::CreateForHandle, nullptr);
	env->AddFunction("PipeWriter", Filter::Pipe::PipeWriter::CreateParams, Filter::Pipe::PipeWriter::Create, nullptr);
	env->AddFunction("PipeWriterForHandle", Filter::Pipe::PipeWriter::CreateForHandleParams, Filter::Pipe::PipeWriter::CreateForHandle, nullptr);
	// Tools
	env->AddFunction("CropDetect", Filter::CropDetect::CreateParams, Filter::CropDetect::Create, nullptr);
	env->AddFunction("HistogramMatching", Filter::HistogramMatching::CreateParams, Filter::HistogramMatching::Create, nullptr);
	env->AddFunction("SequentialToSeekable", Filter::SequentialToSeekable::CreateParams, Filter::SequentialToSeekable::Create, nullptr);
	return nullptr;
}



#include <Shellapi.h>

enum {
	IDD_DONATION = 1,
	ID_SENDEMAIL = 1,
	ID_CLOSE = 2,
};

INT_PTR CALLBACK DlgMsgHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CLOSE:
		close:
			EndDialog(hwnd, 0);
			break;
		case WM_COMMAND:
			switch (wParam) {
				case ID_SENDEMAIL:
					ShellExecute(hwnd, TEXT("open"), TEXT("mailto:napadovskiy@gmail.com?subject=sub3d%20plugin"), TEXT(""), TEXT(""), SW_SHOWNORMAL);
					break;
				case ID_CLOSE:
					goto close;
			}
			break;
	}
	return 0;
}

extern "C" __declspec(dllexport)
void CALLBACK Donate(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	DialogBoxParam(Tools::WinApi::hInstance, (LPCTSTR)IDD_DONATION, hwnd, DlgMsgHandler, 0);
}
