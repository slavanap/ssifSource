#include "libfileread.hpp"

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
