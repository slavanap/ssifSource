#pragma once
#include <iostream>
#include <memory>

class UniversalFileStream : virtual std::shared_ptr<std::streambuf>, public std::istream {
public:
	UniversalFileStream(const char* filename);
};
