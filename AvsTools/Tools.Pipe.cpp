//
// WinAPI pipe tools 
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
#include "Tools.Pipe.hpp"
#include "Tools.WinApi.hpp"

namespace Tools {
	
	using namespace WinApi;

	namespace Pipe {

		ProxyThread::ProxyThread(LPCSTR name_read, LPCSTR name_write) :
			hWrite(nullptr)
		{
			hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, PIPE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			hWrite = CreateNamedPipeA(name_write, PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			if (hRead == INVALID_HANDLE_VALUE || hWrite == INVALID_HANDLE_VALUE) {
				error = true;
				return;
			}
			DWORD ThreadID = 0;
			hThread = CreateThread(nullptr, 0, ThreadProc, this, 0, &ThreadID);
			if (hThread == nullptr)
				error = true;
		}

		ProxyThread::~ProxyThread() {
			stopEvent = true;
			if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
				TerminateThread(hThread, 0);
			CloseHandle(hRead);
			CloseHandle(hWrite);
		}

		DWORD WINAPI ProxyThread::ThreadProc(LPVOID param) {
			auto object = static_cast<ProxyThread*>(param);
			ConnectNamedPipe(object->hRead, nullptr);
			ConnectNamedPipe(object->hWrite, nullptr);
			int flag;
			char buffer[READ_CHUNK_SIZE];
			while (!object->stopEvent) {
				DWORD did_read, did_write;
				flag = ReadFile(object->hRead, buffer, READ_CHUNK_SIZE, &did_read, nullptr);
				if (!flag) {
					DWORD code = GetLastError();
					if (code != ERROR_BROKEN_PIPE)
						ConsolePrintErrorA("Error reading pipe!", code);
					object->error = true;
					break;
				}
				flag = WriteFile(object->hWrite, buffer, did_read, &did_write, nullptr);
				if (!flag) {
					ConsolePrintErrorA("Error writing to pipe!", GetLastError());
					object->error = true;
					break;
				}
				if (did_read != did_write) {
					ConsolePrintErrorA("Written size mismatch!", GetLastError());
					object->error = true;
					break;
				}
			}
			DisconnectNamedPipe(object->hRead);
			FlushFileBuffers(object->hWrite);
			DisconnectNamedPipe(object->hWrite);
			return 0;
		}



		CloneThread::CloneThread(LPCSTR name_read, LPCSTR name_write1, LPCSTR name_write2) :
			hWrite1(nullptr), hWrite2(nullptr)
		{
			hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, CLONE_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			hWrite1 = CreateNamedPipeA(name_write1, PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, CLONE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			hWrite2 = CreateNamedPipeA(name_write2, PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, CLONE_BUFFER_SIZE, 0, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			if (hWrite2 == INVALID_HANDLE_VALUE)
				hWrite2 = CreateFileA(name_write2, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if (hRead == INVALID_HANDLE_VALUE || hWrite1 == INVALID_HANDLE_VALUE || hWrite2 == INVALID_HANDLE_VALUE) {
				error = true;
				return;
			}
			DWORD ThreadID = 0;
			hThread = CreateThread(nullptr, 0, ThreadProc, this, 0, &ThreadID);
			if (hThread == nullptr)
				error = true;
		}

		CloneThread::~CloneThread() {
			stopEvent = true;
			if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
				TerminateThread(hThread, 0);
			CloseHandle(hRead);
			CloseHandle(hWrite1);
			CloseHandle(hWrite2);
		}

		DWORD WINAPI CloneThread::ThreadProc(LPVOID param) {
			auto object = static_cast<CloneThread*>(param);
			ConnectNamedPipe(object->hRead, nullptr);
			ConnectNamedPipe(object->hWrite1, nullptr);
			ConnectNamedPipe(object->hWrite2, nullptr);
			int flag1, flag2;
			char buffer[READ_CHUNK_SIZE];
			while (!object->stopEvent) {
				DWORD did_read, did_write, read_part1, read_part2;
				int errors = 0;
				flag1 = ReadFile(object->hRead, buffer, READ_CHUNK_SIZE, &did_read, nullptr);
				if (!flag1) {
					DWORD code = GetLastError();
					if (code != ERROR_BROKEN_PIPE)
						ConsolePrintErrorA("Error reading pipe!", code);
					object->error = true;
					break;
				}

				read_part1 = did_read / 2;
				read_part2 = did_read - read_part1;
				if (read_part1 > 0) {
					flag1 = WriteFile(object->hWrite1, buffer, read_part1, &did_write, nullptr);
					if (read_part1 != did_write) {
						ConsolePrintErrorA("Written size mismatch!", GetLastError());
						flag1 = false;
					}
					else
						if (!flag1) {
							ConsolePrintErrorA("Error writing to pipe!", GetLastError());
						}
					flag2 = WriteFile(object->hWrite2, buffer, read_part1, &did_write, nullptr);
					if (read_part1 != did_write) {
						ConsolePrintErrorA("Written size mismatch!", GetLastError());
						flag2 = false;
					}
					else
						if (!flag2) {
							ConsolePrintErrorA("Error writing to pipe!", GetLastError());
						}
					if (!flag1 && !flag2) {
						object->error = true;
						break;
					}
				}
				if (read_part2 > 0) {
					flag1 = WriteFile(object->hWrite1, buffer + read_part1, read_part2, &did_write, nullptr);
					if (read_part2 != did_write) {
						ConsolePrintErrorA("Written size mismatch!", GetLastError());
						flag1 = false;
					}
					else
						if (!flag1) {
							ConsolePrintErrorA("Error writing to pipe!", GetLastError());
						}
					flag2 = WriteFile(object->hWrite2, buffer + read_part1, read_part2, &did_write, nullptr);
					if (read_part2 != did_write) {
						ConsolePrintErrorA("Written size mismatch!", GetLastError());
						flag2 = false;
					}
					else
						if (!flag2) {
							ConsolePrintErrorA("Error writing to pipe!", GetLastError());
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



		NullThread::NullThread(LPCSTR name_read) {
			hRead = CreateNamedPipeA(name_read, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			if (hRead == INVALID_HANDLE_VALUE) {
				error = true;
				return;
			}
			DWORD ThreadID = 0;
			hThread = CreateThread(nullptr, 0, ThreadProc, this, 0, &ThreadID);
			if (hThread == nullptr)
				error = true;
		}

		NullThread::~NullThread() {
			stopEvent = true;
			if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
				TerminateThread(hThread, 0);
			CloseHandle(hRead);
		}

		DWORD WINAPI NullThread::ThreadProc(LPVOID param) {
			auto object = static_cast<NullThread*>(param);
			ConnectNamedPipe(object->hRead, nullptr);
			int flag;
			char buffer[READ_CHUNK_SIZE];
			while (!object->stopEvent) {
				DWORD did_read;
				flag = ReadFile(object->hRead, buffer, READ_CHUNK_SIZE, &did_read, nullptr);
				if (!flag) {
					DWORD code = GetLastError();
					if (code != ERROR_BROKEN_PIPE)
						ConsolePrintErrorA("Error reading pipe!", code);
					object->error = true;
					break;
				}
			}
			DisconnectNamedPipe(object->hRead);
			return 0;
		}



		FrameSeparator::FrameSeparator(LPCSTR name, int chunkSize) :
			chunkSize(chunkSize)
		{
			hRead = CreateNamedPipeA(name, PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_WAIT, 1, chunkSize * 10, chunkSize * 10, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
			heDataReady = CreateEvent(nullptr, false, false, nullptr);
			heDataParsed = CreateEvent(nullptr, false, true, nullptr);
			buffer = new char[chunkSize];
			if (hRead == INVALID_HANDLE_VALUE || heDataReady == nullptr || heDataParsed == nullptr || buffer == nullptr) {
				error = true;
				return;
			}
			DWORD ThreadID = 0;
			hThread = CreateThread(nullptr, 0, &ThreadProc, this, 0, &ThreadID);
			if (hThread == nullptr)
				error = true;
		}

		FrameSeparator::~FrameSeparator() {
			stopEvent = true;
			SetEvent(heDataParsed);
			if (WaitForSingleObject(hThread, WAIT_TIME) != WAIT_OBJECT_0)
				TerminateThread(hThread, 0);
			CloseHandle(heDataReady);
			CloseHandle(heDataParsed);
			CloseHandle(hRead);
			delete[] buffer;
		}

		void FrameSeparator::WaitForData() {
			if (error) return;
			WaitForSingleObject(heDataReady, INFINITE);
			ResetEvent(heDataReady);
		}

		void FrameSeparator::DataParsed() {
			if (error) return;
			SetEvent(heDataParsed);
		}

		DWORD WINAPI FrameSeparator::ThreadProc(LPVOID param) {
			FrameSeparator* object = (FrameSeparator*)param;
			ConnectNamedPipe(object->hRead, nullptr);
			while (!object->stopEvent) {
				int want_read = object->chunkSize;
				char *start = (char*)object->buffer;
				WaitForSingleObject(object->heDataParsed, INFINITE);
				ResetEvent(object->heDataParsed);
				int flag = TRUE;
				while (want_read > 0) {
					DWORD did_read;
					flag = ReadFile(object->hRead, start, want_read, &did_read, nullptr);
					if (!flag) {
						DWORD code = GetLastError();
						if (code != ERROR_BROKEN_PIPE)
							ConsolePrintErrorA("Error reading pipe!", code);
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

	}
}
