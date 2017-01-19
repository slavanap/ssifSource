#pragma once

#ifdef LIBUDFREAD_EXPORTS
#define LIBUDFREAD_API __declspec(dllexport)
#else
#define LIBUDFREAD_API __declspec(dllimport)
#endif

#include <streambuf>

struct udfread;
typedef struct udfread_file UDFFILE;

class LIBUDFREAD_API dvdfilebuf : public std::streambuf {
public:
	dvdfilebuf(const char* filename);
	~dvdfilebuf();
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
	std::streampos seekpos(std::streampos sp, std::ios_base::openmode which) override;
	int underflow() override;

	dvdfilebuf(const dvdfilebuf& other) = delete;
	dvdfilebuf& operator=(const dvdfilebuf& other) = delete;
private:
	udfread *_udf;
	udfread_file *_file;
	std::streamoff _size;
	char *_buffer;

	std::streamoff getpos();
};
