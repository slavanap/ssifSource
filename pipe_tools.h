#pragma once
#include "stdafx.h"

#define BUFFER_SIZE 0x80000
#define CLONEPIPE_BUFFER_SIZE (BUFFER_SIZE*5)
#define WAIT_TIME 2000

// Pipes' classes

class PipeThread {
public:
	virtual ~PipeThread() {}
};

class PipeDupThread: public PipeThread {
	volatile bool stop_event;
	static DWORD WINAPI ThreadProc(LPVOID param);
public:
	HANDLE hRead, hWrite;
	HANDLE hThread;
	bool error;
	PipeDupThread(const char* name_read, const char* name_write);
	~PipeDupThread();
};

class PipeCloneThread: public PipeThread {
	volatile bool stop_event;
	static DWORD WINAPI ThreadProc(LPVOID param);
public:
	HANDLE hRead, hWrite1, hWrite2;
	HANDLE hThread;
	bool error;
	PipeCloneThread(const char* name_read, const char* name_write1, const char* name_write2);
	~PipeCloneThread();
};

class NullPipeThread: public PipeThread {
	volatile bool stop_event;
	static DWORD WINAPI ThreadProc(LPVOID param);
public:
	HANDLE hRead;
	HANDLE hThread;
	bool error;
	NullPipeThread(const char* name_read);
	~NullPipeThread();
};

class FrameSeparator: public PipeThread {
	HANDLE heDataReady; 
	HANDLE heDataParsed;
	volatile bool stop_event;
	static DWORD WINAPI ThreadProc(LPVOID param);
public:
	HANDLE hRead;
	HANDLE hThread;
	volatile bool error;
	volatile char *buffer;
	int size;

	FrameSeparator(const char* name, int size);
	~FrameSeparator();
	void WaitForData();
	void DataParsed();
};
