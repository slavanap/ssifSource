// Motion estimation algorithm
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

#pragma once
#include "Tools.AviSynth.Frame.hpp"

namespace Tools {

	struct Point {
		int x, y;

		Point(int x, int y) :
			x(x), y(y)
		{
		}

		bool IsInside(int left, int top, int right, int bottom) const {
			return left <= x && x <= right && top <= y && y <= bottom;
		}

		Point& operator+=(const Point& other) {
			x += other.x, y += other.y;
			return *this;
		}
		Point& operator-=(const Point& other) {
			x -= other.x, y -= other.y;
			return *this;
		}

		Point operator+(const Point& other) const {
			return Point(x + other.x, y + other.y);
		}
		Point operator-(const Point& other) const {
			return Point(x - other.x, y - other.y);
		}

	};

	template<typename T>
	class Array2D : public std::vector<T> {
	public:
		Array2D() { }
		Array2D(size_t width, size_t height) : m_width(width), m_height(height) {
			resize(width, height);
		}
		reference operator()(size_t x, size_t y) {
			assert(0 <= x && x < m_width && 0 <= y && y < m_height);
			return at(y * m_width + x);
		}
		const_reference operator()(size_t x, size_t y) const {
			assert(0 <= x && x < m_width && 0 <= y && y < m_height);
			return at(y * m_width + x);
		}
		void resize(size_t width, size_t height) {
			std::vector<T>::resize(width * height);
			m_width = width, m_height = height;
		}
		void resize(size_t width, size_t height, const value_type& val) {
			std::vector<T>::resize(width * height, val);
			m_width = width, m_height = height;
		}
		size_t width() const { return m_width; }
		size_t height() const { return m_height; }
	private:
		size_t m_width;
		size_t m_height;
	};

	namespace Motion {

		enum {
			BLOCK_SIZE = 16,
		};

		struct Vector {
			int confidence;
			int dx, dy;
			Vector() : confidence(INT_MAX), dx(0), dy(0) {}

			void AssignIfBetter(const Vector& other) {
				if (other.confidence < confidence)
					*this = other;
			}
			bool isSamePoint(const Vector& other) const {
				return dx == other.dx && dy == other.dy;
			}
		};

		typedef Array2D<Vector> Map;

		void Estimate(const AviSynth::Frame& from, AviSynth::Frame& to, Map& result, const Array2D<char>& mask);
		double GetRealConfidence(const Vector& mv);

	}
}
