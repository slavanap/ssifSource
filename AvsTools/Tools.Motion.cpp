//
// Motion estimation algorithm
// Copyright 2016 Vyacheslav Napadovsky.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include "stdafx.h"
#include "Tools.Motion.hpp"
#include "common.h"

namespace Tools {
	namespace Motion {

		using Tools::AviSynth::Frame;

		enum {
			BLOCK_COMPARE_SIZE = BLOCK_SIZE,
			STEPS = 12,
			STARTPATTERN = 2,
		};

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
				if ((abs(dx) > move_x_threshold)
					|| (pixel_x + dx < 0) || (pixel_x + dx + BLOCK_COMPARE_SIZE >= from.width())
					|| (pixel_y + dy < 0) || (pixel_y + dy + BLOCK_COMPARE_SIZE >= from.height())
					|| (pixel_x < 0) || (pixel_x + BLOCK_COMPARE_SIZE >= from.width())
					|| (pixel_y < 0) || (pixel_y + BLOCK_COMPARE_SIZE >= from.height())
					)
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
				// diamond pattern
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
					for (int x = 0; x < (int)map_dest.width(); ++x) {
						if (mask(x, y)) {
							map_dest(x, y) = ProcessBlock(map_source, x, y);
						}
					}
				}

			}
		};

		std::vector<Pattern> Estimation::patterns;

		inline void PrepareFrames(const Frame& left, Frame& right);

		void Estimate(const Frame& from, const Frame& to, Map& result, const Array2D<char>& mask) {
			Frame new_to(to);
			PrepareFrames(from, new_to);
			Map m(result);
			Estimation(from, new_to, m, result, mask);
		}

	}
}


#ifdef MOTION_SIMPLE
namespace Tools {
	namespace Motion {

		inline void PrepareFrames(const Frame& left, Frame& right) { }

		inline void Estimation::NewCandidate(int pixel_x, int pixel_y, Vector& v) {
			v.confidence = 0;
			for (int y = 0; y < BLOCK_COMPARE_SIZE; ++y) {
				for (int x = 0; x < BLOCK_COMPARE_SIZE; ++x) {
					IntPixel temp = from.read(pixel_x + v.dx + x, pixel_y + v.dy + y);
					temp -= to.read(pixel_x + x, pixel_y + y);
					v.confidence += abs(temp.r) + abs(temp.g) + abs(temp.b);
				}
			}
		}

		double GetRealConfidence(const Vector& mv) {
			double c = (double)mv.confidence / (
				Frame::ColorCount           // abs(color - color)
				* 3                         // 3 components
				* BLOCK_COMPARE_SIZE        // loop over block sz * sz
				* BLOCK_COMPARE_SIZE
				);
			return std::max(0.0, std::min(1.0 - c, 1.0));
		}

	}
}
#else

#include "Filter.HistogramMatching.hpp"

namespace Tools {
	namespace Motion {

		/* In case of publishing papers based on this method, please refer to:

			A. Voronov, D. Vatolin, D. Sumin, V. Napadovsky, A. Borisov
			"Towards Automatic Stereo-video Quality Assessment and Detection of Color and Sharpness Mismatch" //
			International Conference on 3D Imaging (IC3D) — 2012. — pp. 1-6. DOI:10.1109/IC3D.2012.6615121
			(section 4.2. Color-Independent Stereo Matching)

			or

			Napadovsky, V. 2014. Locally adaptive algorithm for detection
			and elimination of color inconsistencies between views of stereoscopic video,
			M.S. Thesis, Moscow State University

		*/

		inline void PrepareFrames(const Frame& left, Frame& right) {
			HistogramMatching(right, left);
		}

		inline int dot(const IntPixel& x, const IntPixel& y) {
			return x.r * y.r + x.g * y.g + x.b * y.b;
		}

		inline void Estimation::NewCandidate(int pixel_x, int pixel_y, Vector& v) {
			IntPixel sum;
			for (int y = 0; y < BLOCK_COMPARE_SIZE; ++y) {
				for (int x = 0; x < BLOCK_COMPARE_SIZE; ++x) {
					// to - from
					sum += to.read(pixel_x + x, pixel_y + y);
					sum -= from.read(pixel_x + v.dx + x, pixel_y + v.dy + y);
				}
			}
			sum /= BLOCK_COMPARE_SIZE * BLOCK_COMPARE_SIZE;
			int diffsum = 0;
			int sad = 0;
			for (int y = 0; y < BLOCK_COMPARE_SIZE; ++y) {
				for (int x = 0; x < BLOCK_COMPARE_SIZE; ++x) {
					// from - to
					IntPixel pixel = from.read(pixel_x + v.dx + x, pixel_y + v.dy + y);
					pixel -= to.read(pixel_x + x, pixel_y + y);
					sad += dot(pixel, pixel);

					pixel += sum;
					diffsum += dot(pixel, pixel);
				}
			}
			v.confidence = diffsum;
			v.confidence += sad;
			//v->confidence += sqr(abs(sum.r) + abs(sum.g) + abs(sum.b));
			//v->confidence += sqr(dy);
		}

		double GetRealConfidence(const Vector& mv) {
			double c = (double)mv.confidence / (
				Frame::ColorCount           // color - color
				* 3                         // 3 component in dot
				* BLOCK_COMPARE_SIZE        // loop over block sz * sz
				* BLOCK_COMPARE_SIZE
				* 2                         // 2 components
				);
			return std::max(0.0, std::min(1.0 - c, 1.0));
		}
	}
}

#endif
