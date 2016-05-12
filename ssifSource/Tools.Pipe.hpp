#pragma once
#include "stdafx.h"

namespace Tools {
	namespace Pipe {

		enum {
			READ_CHUNK_SIZE = 0x80000,					// read chunk size
			PIPE_BUFFER_SIZE = 0x1000000,				// request queue site from OS
			CLONE_BUFFER_SIZE = PIPE_BUFFER_SIZE,		// same for CloneThread
			WAIT_TIME = 10000,							// in ms, for hangs detection
		};

		class Thread {
		public:
			HANDLE hRead;
			HANDLE hThread;
			Thread() : hRead(nullptr), hThread(nullptr), error(false), stopEvent(false) { }
			virtual ~Thread() { }
			operator bool() { return !error; }
		protected:
			volatile bool error;
			volatile bool stopEvent;
		};

		class ProxyThread : public Thread {
		public:
			HANDLE hWrite;
			ProxyThread(LPCSTR name_read, LPCSTR name_write);
			~ProxyThread();
		private:
			static DWORD WINAPI ThreadProc(LPVOID param);
		};

		class CloneThread : public Thread {
		public:
			HANDLE hWrite1, hWrite2;
			CloneThread(LPCSTR name_read, LPCSTR name_write1, LPCSTR name_write2);
			~CloneThread();
		private:
			static DWORD WINAPI ThreadProc(LPVOID param);
		};

		class NullThread : public Thread {
		public:
			NullThread(LPCSTR name_read);
			~NullThread();
		private:
			static DWORD WINAPI ThreadProc(LPVOID param);
		};

		class FrameSeparator : public Thread {
		public:
			volatile char *buffer;
			int chunkSize;

			FrameSeparator(const char* name, int chunkSize);
			~FrameSeparator();
			void WaitForData();
			void DataParsed();
		private:
			HANDLE heDataReady;
			HANDLE heDataParsed;
			static DWORD WINAPI ThreadProc(LPVOID param);
		};

	}
}
