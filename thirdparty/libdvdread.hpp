#pragma once

#ifdef LIBDVDREAD_EXPORTS
#define LIBDVDREAD_API __declspec(dllexport)
#else
#define LIBDVDREAD_API __declspec(dllimport)
#endif

#include <streambuf>
#pragma once
#include <streambuf>

typedef struct dvd_reader_s dvd_reader_t;
typedef struct dvd_file_s dvd_file_t;

class LIBDVDREAD_API dvdfilebuf : public std::streambuf {
public:
	dvdfilebuf(const char* filename);
	~dvdfilebuf();
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
	std::streampos seekpos(std::streampos sp, std::ios_base::openmode which) override;
	int underflow() override;

	dvdfilebuf(const dvdfilebuf& other) = delete;
	dvdfilebuf& operator=(const dvdfilebuf& other) = delete;
private:
	dvd_reader_t *_dvd;
	dvd_file_t *_file;
	std::streamoff _size;
	char *_buffer;

	std::streamoff getpos();
};
