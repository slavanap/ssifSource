#pragma once
#include "stdafx.h"
#include "Filter.Pipe.hpp"
#include "Tools.Pipe.hpp"
#include "Tools.WinApi.hpp"

using namespace Tools::Pipe;
using namespace Tools::WinApi;

namespace Filter {

	namespace Pipe {

		LPCSTR CreateReadPipeParams = "[pipe_name]s";

		AVSValue __cdecl CreateReadPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
			LPCSTR pipe_name = args[0].AsString();
			HANDLE hPipe = CreateNamedPipeA(pipe_name, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, PIPE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			if (hPipe == INVALID_HANDLE_VALUE)
				env->ThrowError("Error creating pipe with name \"%s\", code 0x%08X", pipe_name, GetLastError());
			return (int)hPipe;
		}

		LPCSTR CreateWritePipeParams = "[pipe_name]s";

		AVSValue __cdecl CreateWritePipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
			LPCSTR pipe_name = args[0].AsString();
			HANDLE hPipe = CreateNamedPipeA(pipe_name, PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, PIPE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			if (hPipe == INVALID_HANDLE_VALUE)
				env->ThrowError("Error creating pipe with name \"%s\", code 0x%08X", pipe_name, GetLastError());
			return (int)hPipe;
		}

		LPCSTR DestroyPipeParams = "[pipe_cookie]i";

		AVSValue __cdecl DestroyPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
			HANDLE hPipe = (HANDLE)args[0].AsInt();
			DisconnectNamedPipe(hPipe);
			FlushFileBuffers(hPipe);
			CloseHandle(hPipe);
			return AVSValue();
		}



		// class PipeWriter
		
		PipeWriter::PipeWriter(IScriptEnvironment* env, PClip clip, const std::string& pipe_name) :
			GenericVideoFilter(clip), flagClose(true)
		{
			hPipe = CreateFileA(pipe_name.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hPipe == INVALID_HANDLE_VALUE)
				env->ThrowError("Error opening write pipe \"%s\", code 0x%08X", pipe_name.c_str(), GetLastError());
			WriteVideoInfoToPipe(env);
		}

		PipeWriter::PipeWriter(IScriptEnvironment* env, PClip clip, HANDLE hPipe) :
			GenericVideoFilter(clip), hPipe(hPipe), flagClose(false)
		{
			ConnectNamedPipe(hPipe, nullptr);
			WriteVideoInfoToPipe(env);
		}

		PipeWriter::~PipeWriter() {
			if (flagClose)
				CloseHandle(hPipe);
		}

		void PipeWriter::WriteVideoInfoToPipe(IScriptEnvironment* env) {
			if (!ConfidentWrite(hPipe, &vi, sizeof(VideoInfo)))
				env->ThrowError("Error writing VideoInfo to pipe, code 0x%08X", GetLastError());
		}

		PVideoFrame WINAPI PipeWriter::GetFrame(int n, IScriptEnvironment* env) {
			PVideoFrame vf = child->GetFrame(n, env);
			VideoFrameBuffer *vfb = vf->GetFrameBuffer();
			if (!ConfidentWrite(hPipe, vfb->GetReadPtr(), vfb->GetDataSize()))
				env->ThrowError("Error writing to pipe %p, code 0x%08X", hPipe, GetLastError());
			return vf;
		}

		LPCSTR PipeWriter::CreateParams = "[pipe_name]s[clip]c";
		AVSValue __cdecl PipeWriter::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
			return new PipeWriter(env, args[1].AsClip(), args[0].AsString());
		}
		
		LPCSTR PipeWriter::CreateForHandleParams = "[pipe_cookie]i[clip]c";
		AVSValue __cdecl PipeWriter::CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env) {
			return new PipeWriter(env, args[1].AsClip(), reinterpret_cast<HANDLE>(args[0].AsInt()));
		}



		// class PipeReader

		PipeReader::PipeReader(IScriptEnvironment* env, const std::string& pipe_name) :
			flagClose(true)
		{
			hPipe = CreateFileA(pipe_name.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hPipe == INVALID_HANDLE_VALUE)
				env->ThrowError("Error opening read pipe \"%s\", code 0x%08X", pipe_name.c_str(), GetLastError());
			ReadVideoInfoFromPipe(env);
		}

		PipeReader::PipeReader(IScriptEnvironment* env, HANDLE hPipe) :
			hPipe(hPipe), flagClose(false)
		{
			ConnectNamedPipe(hPipe, nullptr);
			ReadVideoInfoFromPipe(env);
		}

		PipeReader::~PipeReader() {
			if (flagClose)
				CloseHandle(hPipe);
		}

		void PipeReader::ReadVideoInfoFromPipe(IScriptEnvironment* env) {
			if (!ConfidentRead(hPipe, &vi, sizeof(VideoInfo)))
				env->ThrowError("Error writing VideoInfo to pipe, code 0x%08X", GetLastError());
		}

		PVideoFrame WINAPI PipeReader::GetFrame(int n, IScriptEnvironment* env) {
			PVideoFrame vf = env->NewVideoFrame(vi);
			VideoFrameBuffer *vfb = vf->GetFrameBuffer();
			if (!ConfidentRead(hPipe, vfb->GetWritePtr(), vfb->GetDataSize()))
				env->ThrowError("Error writing to pipe #%d, code 0x%08X", (int)hPipe, GetLastError());
			return vf;
		}

		LPCSTR PipeReader::CreateParams = "[pipe_name]s";
		AVSValue __cdecl PipeReader::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
			return new PipeReader(env, args[0].AsString());
		}

		LPCSTR PipeReader::CreateForHandleParams = "[pipe_cookie]i";
		AVSValue __cdecl PipeReader::CreateForHandle(AVSValue args, void* user_data, IScriptEnvironment* env) {
			return new PipeReader(env, reinterpret_cast<HANDLE>(args[0].AsInt()));
		}

	}
}
