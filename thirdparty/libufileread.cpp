#include "libufileread.h"

#include <fstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>


struct UdfFileBufLib {
	typedef std::streambuf* (*FCreateUdfFileBuf)(const char* filename);
	typedef void(*FDeleteUdfFileBuf)(std::streambuf* ptr);

	FCreateUdfFileBuf CreateUdfFileBuf;
	FDeleteUdfFileBuf DeleteUdfFileBuf;
	HMODULE hModule;

	UdfFileBufLib() :
		CreateUdfFileBuf(nullptr),
		DeleteUdfFileBuf(nullptr)
	{
		hModule = LoadLibrary(TEXT("libudfread.dll"));
		if (hModule == nullptr)
			throw std::runtime_error("Can't load libudfread.dll");
		CreateUdfFileBuf = (FCreateUdfFileBuf)GetProcAddress(hModule, "CreateUdfFileBuf");
		DeleteUdfFileBuf = (FDeleteUdfFileBuf)GetProcAddress(hModule, "DeleteUdfFileBuf");
		if (CreateUdfFileBuf == nullptr || DeleteUdfFileBuf == nullptr) {
			FreeLibrary(hModule);
			throw std::runtime_error("libudfread.dll has unknown version");
		}
	}

	~UdfFileBufLib() {
		FreeLibrary(hModule);
	}
};

static std::unique_ptr<UdfFileBufLib> udfFileBufLib;

inline std::shared_ptr<std::streambuf> CreateFileBuf(const char* filename) {
	// try open file first
	{
		auto file = std::make_shared<std::filebuf>();
		if (file->open(filename, std::ios_base::in | std::ios_base::binary) != nullptr)
			return file;
	}
	
	// next try UDF if possible
	if (udfFileBufLib == nullptr)
		udfFileBufLib.reset(new UdfFileBufLib());
	auto udfBuf = udfFileBufLib->CreateUdfFileBuf(filename);
	if (udfBuf == nullptr)
		throw std::runtime_error("Can't open UDF image");
	return std::shared_ptr<std::streambuf>(udfBuf, [](std::streambuf* m) { udfFileBufLib->DeleteUdfFileBuf(m); });
}


UniversalFileStream::UniversalFileStream(const char* filename) :
	std::shared_ptr<std::streambuf>(CreateFileBuf(filename)), std::istream(std::shared_ptr<std::streambuf>::get())
{
	// empty
}



size_t udfread(void* buffer, size_t elementSize, size_t elementCount, UDFFILE stream) {
	auto sz = elementSize * elementCount;
	auto ret = stream->rdbuf()->sgetn((char*)buffer, sz);
	return (ret < 0) ? 0 : (size_t)ret;
}

long udffile_get_length(UDFFILE stream) {
	auto buf = stream->rdbuf();
	auto pos = buf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
	auto ret = buf->pubseekoff(0, std::ios_base::end, std::ios_base::in);
	buf->pubseekpos(pos, std::ios_base::in);
	return (long)ret;
}

UDFFILE udfopen(const char* path, const char* mode) {
	if (!strcmp(mode, "rb") || !strcmp(mode, "br") || !strcmp(mode, "b")) {
		try { return new UniversalFileStream(path); } catch (...) {}
	}
	return nullptr;
}

int udfclose(UDFFILE stream) {
	delete stream;
	return 0;
}
