#pragma once

#ifdef __cplusplus

#include <iostream>
#include <memory>

class UniversalFileStream : virtual std::shared_ptr<std::streambuf>, public std::istream {
public:
	UniversalFileStream(const char* filename);
};

typedef UniversalFileStream* UDFFILE;
extern "C" {
#else
typedef void* UDFFILE;
#endif

size_t udfread(void* buffer, size_t elementSize, size_t elementCount, UDFFILE stream);
long udffile_get_length(UDFFILE stream);
UDFFILE udfopen(const char* path, const char* mode);
int udfclose(UDFFILE stream);

#ifdef __cplusplus
}
#endif