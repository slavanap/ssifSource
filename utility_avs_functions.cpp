#pragma once
#include "stdafx.h"
#include "utility_avs_functions.h"

#define PIPE_BUFFER_SIZE 0x1000000

AVSValue __cdecl ssCreateReadPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *pipe_name = args[0].AsString();
	HANDLE hPipe = CreateNamedPipeA(pipe_name, PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, PIPE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
		env->ThrowError("Error creating pipe with name \"%s\", code 0x%08X", pipe_name, GetLastError());
	return (int)hPipe;
}

AVSValue __cdecl ssCreateWritePipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
	const char *pipe_name = args[0].AsString();
	HANDLE hPipe = CreateNamedPipeA(pipe_name, PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, PIPE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
		env->ThrowError("Error creating pipe with name \"%s\", code 0x%08X", pipe_name, GetLastError());
	return (int)hPipe;
}

AVSValue __cdecl ssDestroyPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
	HANDLE hPipe = (HANDLE)args[0].AsInt();
	DisconnectNamedPipe(hPipe);
	FlushFileBuffers(hPipe);
	CloseHandle(hPipe);
	return AVSValue();
}

bool ConfidentRead(HANDLE hFile, void* buffer, size_t cbSize) {
	char *buf = (char*)buffer;
	if (cbSize == 0) return true;
	if (cbSize < 0) return false;
	DWORD temp;
	do {
		if (!ReadFile(hFile, buf, cbSize, &temp, NULL))
			return false;
		cbSize -= temp;
		buf += temp;
	} while (cbSize > 0);
	return true;
}

bool ConfidentWrite(HANDLE hFile, const void* buffer, size_t cbSize) {
	const char *buf = (char*)buffer;
	if (cbSize == 0) return true;
	if (cbSize < 0) return false;
	DWORD temp;
	do {
		if (!WriteFile(hFile, buf, cbSize, &temp, NULL))
			return false;
		cbSize -= temp;
		buf += temp;
	} while (cbSize > 0);
	return true;
}


class cWriteToPipe: public GenericVideoFilter {
private:
	HANDLE hPipe;
	bool flag_close;

	void writeVideoInfo(IScriptEnvironment* env) {
		if (!ConfidentWrite(hPipe, &vi, sizeof(VideoInfo)))
			env->ThrowError("Error writing VideoInfo to pipe, code 0x%08X", GetLastError());
	}

public:
	cWriteToPipe(IScriptEnvironment* env, PClip clip, const string& pipe_name): 
	  GenericVideoFilter(clip), flag_close(true)
	{
		hPipe = CreateFileA(pipe_name.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hPipe == INVALID_HANDLE_VALUE)
			env->ThrowError("Error opening write pipe \"%s\", code 0x%08X", pipe_name.c_str(), GetLastError());
		writeVideoInfo(env);
	}
	cWriteToPipe(IScriptEnvironment* env, PClip clip, HANDLE hPipe):
	  GenericVideoFilter(clip), hPipe(hPipe), flag_close(false)
	{
		ConnectNamedPipe(hPipe, NULL);
		writeVideoInfo(env);
	}
	virtual ~cWriteToPipe() {
		if (flag_close)
			CloseHandle(hPipe);
	}

	// AviSynth virtual functions
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) {
		PVideoFrame vf = child->GetFrame(n, env);
		VideoFrameBuffer *vfb = vf->GetFrameBuffer();
		if (!ConfidentWrite(hPipe, vfb->GetReadPtr(), vfb->GetDataSize()))
			env->ThrowError("Error writing to pipe #%d, code 0x%08X", (int)hPipe, GetLastError());
		return vf;
	}
};

AVSValue __cdecl ssWriteToPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new cWriteToPipe(env, args[1].AsClip(), args[0].AsString());
}

AVSValue __cdecl ssWriteToPipeHandle(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new cWriteToPipe(env, args[1].AsClip(), (HANDLE)args[0].AsInt());
}

class cReadFromPipe: public IClip {
private:
	HANDLE hPipe;
	bool flag_close;
	VideoInfo vi;

	void readVideoInfo(IScriptEnvironment* env) {
		if (!ConfidentRead(hPipe, &vi, sizeof(VideoInfo)))
			env->ThrowError("Error writing VideoInfo to pipe, code 0x%08X", GetLastError());
	}

public:
	cReadFromPipe(IScriptEnvironment* env, const string& pipe_name): flag_close(true)
	{
		hPipe = CreateFileA(pipe_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hPipe == INVALID_HANDLE_VALUE)
			env->ThrowError("Error opening read pipe \"%s\", code 0x%08X", pipe_name.c_str(), GetLastError());
		readVideoInfo(env);
	}
	cReadFromPipe(IScriptEnvironment* env, HANDLE hPipe): hPipe(hPipe), flag_close(false)
	{
		ConnectNamedPipe(hPipe, NULL);
		readVideoInfo(env);
	}
	virtual ~cReadFromPipe() {
		if (flag_close)
			CloseHandle(hPipe);
	}

	// AviSynth virtual functions
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) {
		PVideoFrame vf = env->NewVideoFrame(vi);
		VideoFrameBuffer *vfb = vf->GetFrameBuffer();
		if (!ConfidentRead(hPipe, vfb->GetWritePtr(), vfb->GetDataSize()))
			env->ThrowError("Error writing to pipe #%d, code 0x%08X", (int)hPipe, GetLastError());
		return vf;
	}

	bool WINAPI GetParity(int n) { return false; }
	void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { /* empty */ }
	const VideoInfo& WINAPI GetVideoInfo() { return vi; }
	void WINAPI SetCacheHints(int cachehints, int frame_range) { /* empty */ }
};

AVSValue __cdecl ssReadFromPipe(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new cReadFromPipe(env, args[0].AsString());
}

AVSValue __cdecl ssReadFromPipeHandle(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new cReadFromPipe(env, (HANDLE)args[0].AsInt());
}
