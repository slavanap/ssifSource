// 3D subtitle rendering tool
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
#include "Tools.Motion.hpp"
#include <common.h>

namespace Tools {
	namespace Motion {

		using Tools::AviSynth::Frame;

		enum {
			BLOCK_COMPARE_SIZE = BLOCK_SIZE,
			STEPS = 12,
			STARTPATTERN = 2,
		};

		static inline int sqr(int x) {
			return x*x;
		}

		struct IntPixel {
			int r, g, b;

			IntPixel() : r(0), g(0), b(0) { }
			IntPixel(int r, int g, int b) : r(r), g(g), b(b) { }
			IntPixel(const Frame::Pixel& fp) : r(fp.r), g(fp.g), b(fp.b) { }

			IntPixel& operator+=(const IntPixel& other) {
				r += other.r, g += other.g, b += other.b;
				return *this;
			}
			IntPixel& operator+=(const Frame::Pixel& other) {
				r += other.r, g += other.g, b += other.b;
				return *this;
			}
			IntPixel& operator-=(const IntPixel& other) {
				r -= other.r, g -= other.g, b -= other.b;
				return *this;
			}
			IntPixel& operator-=(const Frame::Pixel& other) {
				r -= other.r, g -= other.g, b -= other.b;
				return *this;
			}
			IntPixel& operator/=(int value) {
				r /= value, g /= value, b /= value;
				return *this;
			}
		};

		typedef std::vector<Point> Pattern;

		class Estimation {
			static std::vector<Pattern> patterns;

			int move_x_threshold;
			const Frame &from, &to;

			void NewCandidate(int pixel_x, int pixel_y, Vector& v);

			void AddCandidate(Vector& candidate, int pixel_x, int pixel_y, int dx, int dy) {
				if ((abs(dx) > move_x_threshold) ||
					!((0 <= pixel_x + dx) && (pixel_x + dx + BLOCK_COMPARE_SIZE < from.width())) ||
					!((0 <= pixel_y + dy) && (pixel_y + dy + BLOCK_COMPARE_SIZE < from.height())))
				{
					return;
				}
				Vector v;
				v.dx = dx, v.dy = dy;
				NewCandidate(pixel_x, pixel_y, v);
				candidate.AssignIfBetter(v);
			}

			void AddPattern(Vector& candidate, int pixel_x, int pixel_y, int dx, int dy, int pattern_number) {
				const Pattern &pattern = patterns[pattern_number];
				for (Pattern::const_iterator it = pattern.begin(); it != pattern.end(); ++it)
					AddCandidate(candidate, pixel_x, pixel_y, it->x + dx, it->y + dy);
			}

			Vector ProcessBlock(const Map& motion_map, int block_x, int block_y) {
				int pixel_x = block_x * BLOCK_SIZE + (BLOCK_SIZE - BLOCK_COMPARE_SIZE);
				int pixel_y = block_y * BLOCK_SIZE + (BLOCK_SIZE - BLOCK_COMPARE_SIZE);
				Vector candidate;
				AddCandidate(candidate, pixel_x, pixel_y, 0, 0);

				for (int block_dx = -1; block_dx <= 1; ++block_dx) {
					for (int block_dy = -1; block_dy <= 1; ++block_dy) {
						int bx = block_x + block_dx;
						int by = block_y + block_dy;
						if (0 <= bx && bx < (int)motion_map.width() &&
							0 <= by && by < (int)motion_map.height())
						{
							const Vector &mv = motion_map(bx, by);
							if (!(mv.dx == 0 && mv.dy == 0)) {
								AddCandidate(candidate, pixel_x, pixel_y, mv.dx, mv.dy);
								AddPattern(candidate, pixel_x, pixel_y, mv.dx, mv.dy, STARTPATTERN);
							}
						}
					}
				}

				int pattern_number = STARTPATTERN;
				Vector last_delta = candidate;
				for (int i = 0; i < STEPS; ++i) {
					AddPattern(candidate, pixel_x, pixel_y, last_delta.dx, last_delta.dy, pattern_number);
					if (last_delta.isSamePoint(candidate)) {
						if (--pattern_number < 0)
							break;
					}
					else {
						pattern_number = STARTPATTERN;
						last_delta = candidate;
					}
				}

				return candidate;
			}

			static void init_patterns() {
				patterns.push_back({
					Point(0, 1), Point(1, 0), Point(0, -1), Point(-1, 0)
				});
				patterns.push_back({
					Point(0, 5), Point(5, 0), Point(0, -5), Point(-5, 0),
					Point(2, 2), Point(2, -2), Point(-2, -2), Point(-2, 2)
				});
				patterns.push_back({
					Point(0, 2), Point(2, 0), Point(0, -2), Point(-2, 0),
					Point(1, 1), Point(1, -1), Point(-1, -1), Point(-1, 1)
				});
			}

		public:
			Estimation(const Frame& from, const Frame& to, const Map& map_source, Map& map_dest, const Array2D<char>& mask) :
				from(from), to(to)
			{
				if (patterns.empty())
					init_patterns();
				assert(
					from.width() == to.width() &&
					from.height() == to.height());
				move_x_threshold = from.width() / 10;
				map_dest.resize(
					DIV_AND_ROUND(from.width(), BLOCK_SIZE),
					DIV_AND_ROUND(from.height(), BLOCK_SIZE));
				assert(
					map_source.width() == map_dest.width() &&
					map_source.height() == map_dest.height());

#pragma omp parallel for schedule(guided)
				for (int y = 0; y < (int)map_dest.height(); ++y) {
					for (size_t x = 0; x < map_dest.width(); ++x) {
						if (mask(x, y)) {
							map_dest(x, y) = ProcessBlock(map_source, x, y);
						}
					}
				}

			}
		};

		std::vector<Pattern> Estimation::patterns;
	}
}

#ifdef MOTION_PROPRIETARY
	#include <Tools.Motion.Proprietary.hpp>
#else
namespace Tools {
	namespace Motion {

		inline void PrepareFrames(const Frame& left, Frame& right) { }

		inline void Estimation::NewCandidate(int pixel_x, int pixel_y, Vector& v) {
			v.confidence = 0;
			for (int y = 0; y < BLOCK_COMPARE_SIZE; ++y) {
				for (int x = 0; x < BLOCK_COMPARE_SIZE; ++x) {
					IntPixel temp = from.read(pixel_x + v.dx + x, pixel_y + v.dy + y);
					temp -= to.read(pixel_x + x, pixel_y + y);
					v.confidence += sqr(abs(temp.r) + abs(temp.g) + abs(temp.b));
				}
			}
		}

		double GetRealConfidence(const Vector& mv) {
			double c = (double)mv.confidence / (
				Frame::ColorCount           // color - color
				* 3                         // 3 color components
				* BLOCK_COMPARE_SIZE        // loop over block sz * sz
				* BLOCK_COMPARE_SIZE
				);
			return std::max(0.0, std::min(1.0 - c, 1.0));
		}

	}
}
#endif

namespace Tools {
	namespace Motion {

		void Estimate(const Frame& from, Frame& to, Map& result, const Array2D<char>& mask) {
			PrepareFrames(from, to);
			Map m(result);
			Estimation(from, to, m, result, mask);
		}

	}
}
