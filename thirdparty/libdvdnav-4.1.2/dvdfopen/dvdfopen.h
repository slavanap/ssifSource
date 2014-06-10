#pragma once

#ifdef DVDFOPEN_EXPORTS
#define DVDFOPEN_API __declspec(dllexport)
#else
#define DVDFOPEN_API __declspec(dllimport)
#endif

class common_file_reader {
public:
	virtual uint64_t read(void* buffer, uint64_t bytes) = 0;
	virtual bool seek(int64_t offset, int seek_type) = 0;
	virtual uint64_t getpos() = 0;
	virtual uint64_t filesize() = 0;
	virtual bool iserror() = 0;
	virtual void free();
};

DVDFOPEN_API common_file_reader* universal_fopen(const wchar_t *filename);
DVDFOPEN_API common_file_reader* universal_fopen(const char *filename);
