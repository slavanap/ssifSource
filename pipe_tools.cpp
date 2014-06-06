#include "stdafx.h"
#include "pipe_tools.h"

void ShowErrorA(LPSTR desc, DWORD code) {
	if (desc == NULL)
		desc = "Error";
	LPVOID lpMsgBuf;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPSTR)&lpMsgBuf, 0, NULL);
	fprintf(stderr, "\n%s | 0x%.8x: %s\n", desc, code, (LPSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}


// === class PipeDupThread ====================================================

DWORD WINAPI PipeDupThread::ThreadProc(LPVOID param) {
	PipeDupThread *object = (PipeDupThread*)param;
	ConnectNamedPipe(object->hRead, NULL);
	ConnectNamedPipe(object->hWrite, NULL);
	int flag;
	char buffer[BUFFER_SIZE];
	while (!object->stop_event) {
		DWORD did_read, did_write;
		flag = ReadFile(object->hRead, buffer, BUFFER_SIZE, &did_read, NULL);
		if (!flag) {
			DWORD code = GetLastError();
			if (code != ERROR_BROKEN_PIPE)
				ShowErrorA("Error reading pipe!", code);
			object->error = true;
			break;
		}
		flag = WriteFile(object->hWrite, buffer, did_read, &did_write, NULL);
		if (!flag) {
			ShowErrorA("Error writing to pipe!", GetLastError());
			object->error = true;
			break;
		}
		if (did_read != did_write) {
			ShowErrorA("Written size mismatch!", GetLastError());
			object->error = true;
			break;
		}
	}
	DisconnectNamedPipe(object->hRead);
	FlushFileBuffers(object->hWrite);
	DisconnectNamedPipe(object->hWrite);
	return 0;
}

PipeDupThread::PipeDupThread(const char* name_read, const char* name_write):
	stop_event(false), error(false), hRead(NULL), hWrite(NULL), hThread(NULL)
{
	hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, CLONEPIPE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	hWrite = CreateNamedPipeA(name_write, PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hRead == INVALID_HANDLE_VALUE || hWrite == INVALID_HANDLE_VALUE) {
		error = true;
		return;
	}
	DWORD ThreadID = 0;
	hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &ThreadID);
	if (hThread == NULL)
		error = true;
}

PipeDupThread::~PipeDupThread() {
	stop_event = true;
	if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
		TerminateThread(hThread, 0);
	CloseHandle(hRead);
	CloseHandle(hWrite);
}



// === class PipeCloneThread ==================================================

DWORD WINAPI PipeCloneThread::ThreadProc(LPVOID param) {
	PipeCloneThread *object = (PipeCloneThread*)param;
	ConnectNamedPipe(object->hRead, NULL);
	ConnectNamedPipe(object->hWrite1, NULL);
	ConnectNamedPipe(object->hWrite2, NULL);
	int flag1, flag2;
	char buffer[BUFFER_SIZE];
	while (!object->stop_event) {
		DWORD did_read, did_write, read_part1, read_part2;
		int errors = 0;
		flag1 = ReadFile(object->hRead, buffer, BUFFER_SIZE, &did_read, NULL);
		if (!flag1) {
			DWORD code = GetLastError();
			if (code != ERROR_BROKEN_PIPE)
				ShowErrorA("Error reading pipe!", code);
			object->error = true;
			break;
		}

		read_part1 = did_read / 2;
		read_part2 = did_read - read_part1;
		if (read_part1 > 0) {
			flag1 = WriteFile(object->hWrite1, buffer, read_part1, &did_write, NULL);
			if (read_part1 != did_write) {
				ShowErrorA("Written size mismatch!", GetLastError());
				flag1 = false;
			} else
			if (!flag1) {
				ShowErrorA("Error writing to pipe!", GetLastError());
			}
			flag2 = WriteFile(object->hWrite2, buffer, read_part1, &did_write, NULL);
			if (read_part1 != did_write) {
				ShowErrorA("Written size mismatch!", GetLastError());
				flag2 = false;
			} else
			if (!flag2) {
				ShowErrorA("Error writing to pipe!", GetLastError());
			}
			if (!flag1 && !flag2) {
				object->error = true;
				break;
			}
		}
		if (read_part2 > 0) {
			flag1 = WriteFile(object->hWrite1, buffer + read_part1, read_part2, &did_write, NULL);
			if (read_part2 != did_write) {
				ShowErrorA("Written size mismatch!", GetLastError());
				flag1 = false;
			} else
			if (!flag1) {
				ShowErrorA("Error writing to pipe!", GetLastError());
			}
			flag2 = WriteFile(object->hWrite2, buffer + read_part1, read_part2, &did_write, NULL);
			if (read_part2 != did_write) {
				ShowErrorA("Written size mismatch!", GetLastError());
				flag2 = false;
			} else
			if (!flag2) {
				ShowErrorA("Error writing to pipe!", GetLastError());
			}
			if (!flag1 && !flag2) {
				object->error = true;
				break;
			}
		}
	}
	DisconnectNamedPipe(object->hRead);
	FlushFileBuffers(object->hWrite1);
	DisconnectNamedPipe(object->hWrite1);
	FlushFileBuffers(object->hWrite2);
	DisconnectNamedPipe(object->hWrite2);
	return 0;
}

PipeCloneThread::PipeCloneThread(const char* name_read, const char* name_write1, const char* name_write2):
	stop_event(false), error(false), hRead(NULL), hWrite1(NULL), hWrite2(NULL), hThread(NULL)
{
	hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, CLONEPIPE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	hWrite1 = CreateNamedPipeA(name_write1, PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, CLONEPIPE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	hWrite2 = CreateNamedPipeA(name_write2, PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, CLONEPIPE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hWrite2 == INVALID_HANDLE_VALUE)
		hWrite2 = CreateFileA(name_write2, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hRead == INVALID_HANDLE_VALUE || hWrite1 == INVALID_HANDLE_VALUE || hWrite2 == INVALID_HANDLE_VALUE) {
		error = true;
		return;
	}
	DWORD ThreadID = 0;
	hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &ThreadID);
	if (hThread == NULL)
		error = true;
}

PipeCloneThread::~PipeCloneThread() {
	stop_event = true;
	if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
		TerminateThread(hThread, 0);
	CloseHandle(hRead);
	CloseHandle(hWrite1);
	CloseHandle(hWrite2);
}



// === class NullPipeThread ===================================================

DWORD WINAPI NullPipeThread::ThreadProc(LPVOID param) {
	NullPipeThread *object = (NullPipeThread*)param;
	ConnectNamedPipe(object->hRead, NULL);
	int flag;
	char buffer[BUFFER_SIZE];
	while (!object->stop_event) {
		DWORD did_read;
		flag = ReadFile(object->hRead, buffer, BUFFER_SIZE, &did_read, NULL);
		if (!flag) {
			DWORD code = GetLastError();
			if (code != ERROR_BROKEN_PIPE)
				ShowErrorA("Error reading pipe!", code);
			object->error = true;
			break;
		}
	}
	DisconnectNamedPipe(object->hRead);
	return 0;
}

NullPipeThread::NullPipeThread(const char* name_read):
	stop_event(false), error(false), hRead(NULL), hThread(NULL)
{
	hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hRead == INVALID_HANDLE_VALUE) {
		error = true;
		return;
	}
	DWORD ThreadID = 0;
	hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &ThreadID);
	if (hThread == NULL)
		error = true;
}

NullPipeThread::~NullPipeThread() {
	stop_event = true;
	if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
		TerminateThread(hThread, 0);
	CloseHandle(hRead);
}



// === class FrameSeparator ===================================================

DWORD WINAPI FrameSeparator::ThreadProc(LPVOID param) {
	FrameSeparator* object = (FrameSeparator*)param;
	ConnectNamedPipe(object->hRead, NULL);
	while (!object->stop_event) {
		int want_read = object->size;
		char *start = (char*)object->buffer;
		WaitForSingleObject(object->heDataParsed, INFINITE);
		ResetEvent(object->heDataParsed);
		int flag;
		while (want_read > 0) {
			DWORD did_read;
			flag = ReadFile(object->hRead, start, want_read, &did_read, NULL);
			if (!flag) {
				DWORD code = GetLastError();
				if (code != ERROR_BROKEN_PIPE)
					ShowErrorA("Error reading pipe!", code);
				break;
			}
			want_read -= did_read;
			start += did_read;
		}
		SetEvent(object->heDataReady);
		if (!flag) {
			object->error = true;
			break;
		}
	}
	DisconnectNamedPipe(object->hRead);
	return 0;
}

FrameSeparator::FrameSeparator(const char* name, int size): 
	stop_event(false), error(false), hThread(NULL), size(size) 
{
	hRead = CreateNamedPipeA(name, PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, size*10, size*10, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	heDataReady = CreateEvent(NULL, false, false, NULL);
	heDataParsed = CreateEvent(NULL, false, true, NULL);
	buffer = new char[size];
	if (hRead == INVALID_HANDLE_VALUE || heDataReady == NULL || heDataParsed == NULL || buffer == NULL) {
		error = true;
		return;
	}
	DWORD ThreadID = 0;
	hThread = CreateThread(NULL, 0, &ThreadProc, this, 0, &ThreadID);
	if (hThread == NULL)
		error = true;
}

FrameSeparator::~FrameSeparator() {
	stop_event = true;
	if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
		TerminateThread(hThread, 0);
	CloseHandle(heDataReady);
	CloseHandle(heDataParsed);
	CloseHandle(hRead);
	delete[] buffer;
}

void FrameSeparator::WaitForData() {
	if (error) return;
	SetEvent(heDataParsed);
	WaitForSingleObject(heDataReady, INFINITE);
	ResetEvent(heDataReady);
}

void FrameSeparator::DataParsed() {
	if (error) return;
}
