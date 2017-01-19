#include "libdvdread.hpp"

#include <algorithm>
#include <vector>
#include <string>

#include <dvdread/dvd_reader.h>

constexpr int default_buffer_size = DVD_VIDEO_LB_LEN * 1024;

dvdfilebuf::dvdfilebuf(const char* filename) :
	_dvd(nullptr),
	_file(nullptr),
	_buffer(nullptr)
{
	std::string dvd_fn(filename);
	std::string file_fn;

	std::replace(dvd_fn.begin(), dvd_fn.end(), '/', '\\');
	size_t pos = dvd_fn.find_first_of('\\');
	while (pos != std::string::npos) {
		dvd_fn[pos] = 0;

		struct _stat64 info;
		if (_stati64(dvd_fn.c_str(), &info) == 0) {
			if ((info.st_mode & S_IFREG) != 0) {
				// found an image
				dvd_fn[pos] = '\\';
				file_fn = dvd_fn.substr(pos);
				std::replace(file_fn.begin(), file_fn.end(), '\\', '/');
				dvd_fn.resize(pos);
				break;
			}
			if ((info.st_mode & S_IFDIR) == 0)
				throw std::runtime_error("invalid file");
		}
		printf("%d\n", errno);
		// else ignore errors of stat
		
		// still directory
		dvd_fn[pos] = '\\';
		pos = dvd_fn.find_first_of('\\', pos + 1);
	}

	try {
		_dvd = DVDOpen(dvd_fn.c_str());
		if (_dvd == nullptr)
			throw std::runtime_error("Can't open the image");
		_file = DVDOpenFilename(_dvd, const_cast<char*>(file_fn.c_str()));
		if (_file == nullptr)
			throw std::runtime_error("Can't open file in the image");
		_size = DVDGetFileSize(_file);
		_buffer = new char[default_buffer_size];
	}
	catch (...) {
		DVDCloseFile(_file);
		DVDClose(_dvd);
		throw;
	}
}

dvdfilebuf::~dvdfilebuf() {
	DVDCloseFile(_file);
	DVDClose(_dvd);
	delete[] _buffer;
}

std::streampos dvdfilebuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
	bool success = false;
	if (which == std::ios_base::in) {
		if (off == 0)
			success = true;
		else {
			switch (way) {
				case std::ios_base::beg:
					success = DVDFileSeek(_file, off) != -1;
					break;
				case std::ios_base::cur:
					success = DVDFileSeek(_file, _size + off) != -1;
					break;
				case std::ios_base::end:
					success = DVDFileSeek(_file, getpos() + off) != -1;
					break;
			}
		}
	}
	return std::streampos(success ? getpos() : std::streamoff(-1));
}

std::streampos dvdfilebuf::seekpos(std::streampos sp, std::ios_base::openmode which) {
	return seekoff(sp, std::ios_base::beg, which);
}

int dvdfilebuf::underflow() {
	if (this->gptr() == this->egptr()) {
		auto bytesLeft = _size - getpos();
		if (bytesLeft > 0) {
			if (bytesLeft > default_buffer_size)
				bytesLeft = default_buffer_size;
			auto ret = DVDReadBytes(_file, &_buffer[0], (size_t)bytesLeft);
			this->setg(&_buffer[0], &_buffer[0], &_buffer[0] + ret);
		}
	}
	return this->gptr() == this->egptr()
		? std::char_traits<char>::eof()
		: std::char_traits<char>::to_int_type(*this->gptr());
}

std::streamoff dvdfilebuf::getpos() {
	return DVDGetFilePos(_file);
}
