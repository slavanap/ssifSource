#include "libudfread.hpp"

#include <algorithm>
#include <vector>
#include <string>

#include "config.h"
#include "udfread.h"

constexpr int default_buffer_size = UDF_BLOCK_SIZE * 1024;

dvdfilebuf::dvdfilebuf(const char* filename) :
	_udf(nullptr),
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
		// else ignore errors of stat
		
		// still directory
		dvd_fn[pos] = '\\';
		pos = dvd_fn.find_first_of('\\', pos + 1);
	}

	try {
		_udf = udfread_init();
		if (_udf == nullptr)
			throw std::bad_alloc();
		if (udfread_open(_udf, dvd_fn.c_str()) < 0)
			throw std::runtime_error("Can't open the image");
		_file = udfread_file_open(_udf, file_fn.c_str());
		if (_file == nullptr)
			throw std::runtime_error("Can't open file in the image");
		_size = udfread_file_size(_file);
		_buffer = new char[default_buffer_size];
	}
	catch (...) {
		udfread_file_close(_file);
		udfread_close(_udf);
		throw;
	}
}

dvdfilebuf::~dvdfilebuf() {
	delete[] _buffer;
	udfread_file_close(_file);
	udfread_close(_udf);
}

std::streampos dvdfilebuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
	bool success = false;
	if (which == std::ios_base::in) {
		int whence;
		switch (way) {
			case std::ios_base::beg:
				whence = UDF_SEEK_SET;
				break;
			case std::ios_base::cur:
				whence = UDF_SEEK_CUR;
				break;
			case std::ios_base::end:
				whence = UDF_SEEK_END;
				break;
			default:
				return std::streamoff(-1);
		}
		return udfread_file_seek(_file, off, whence);
	}
	return std::streamoff(-1);
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
			auto ret = udfread_file_read(_file, &_buffer[0], (size_t)bytesLeft);
			if (ret < 0)
				throw std::runtime_error("error in udfread_file_read");
			this->setg(&_buffer[0], &_buffer[0], &_buffer[0] + ret);
		}
	}
	return this->gptr() == this->egptr()
		? std::char_traits<char>::eof()
		: std::char_traits<char>::to_int_type(*this->gptr());
}

std::streamoff dvdfilebuf::getpos() {
	return udfread_file_tell(_file);
}
