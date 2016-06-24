// AviSynth VideoFrame wrapper
// Copyright 2016 Vyacheslav Napadovsky.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Tools.AviSynth.Frame.hpp"

namespace Tools {
	namespace AviSynth {

		bool Frame::operator!=(const Frame& other) const {
			if (width() != other.width() || height() != other.height())
				return true;
			size_t line_size = width() * sizeof(Pixel);
			for (int y = height() - 1; y >= 0; --y) {
				const Pixel &line = read(0, y), &other_line = other.read(0, y);
				if (memcmp(&line, &other_line, line_size) != 0)
					return true;
			}
			return false;
		}

		void Frame::EnableWriting() {
			if (m_env == nullptr)
				throw std::runtime_error("Can't enable writing for empty frame");

			if (m_write_ptr != nullptr)
				return;

			Frame copy(m_env, m_vi);
			size_t line_size = width() * sizeof(Pixel);
			for (int y = 0; y < height(); ++y) {
				const Pixel &line = read_raw(0, y);
				Pixel &copy_line = copy.write_raw(0, y);
				memcpy(&copy_line, &line, line_size);
			}
			*this = copy;
		}

		void Frame::Dump(const std::string& filename) const {
			std::ofstream f(filename.c_str());
			for (int y = 0; y < height(); ++y) {
				for (int x = 0; x < width(); ++x) {
					auto &px = read(x, y);
					f << (int)px.r << '/' << (int)px.g << '/' << (int)px.b << ',';
				}
				f << std::endl;
			}
		}

		void Frame::DumpModified(const std::string& filename) {
			std::ofstream f(filename.c_str());
			for (int y = 0; y < height(); ++y) {
				for (int x = 0; x < width(); ++x) {
					auto &px = write(x, y);
					f << (int)px.r << '/' << (int)px.g << '/' << (int)px.b << ',';
				}
				f << std::endl;
			}
		}

	}
}
