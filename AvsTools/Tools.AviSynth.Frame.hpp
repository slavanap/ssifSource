//
// AviSynth VideoFrame wrapper
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


#pragma once
#include "Tools.AviSynth.hpp"

namespace Tools {
	namespace AviSynth {

		// simplifies pixel-based access to PVideoFrame

		class Frame {
		public:

			enum {
				ColorCount = 0x100,
			};

#pragma pack(push,1)
			struct Pixel {
				BYTE b;
				BYTE g;
				BYTE r;
				BYTE a;
				Pixel() { }
				Pixel(BYTE r, BYTE g, BYTE b, BYTE a = 0xFF) : r(r), g(g), b(b), a(a) { }

				Pixel(int r, int g, int b, int a = 0xFF) :
					r(ColorLimits(r)),
					g(ColorLimits(g)),
					b(ColorLimits(b)),
					a(ColorLimits(a))
				{
					// empty
				}
				Pixel(float r, float g, float b, float a = 1.0f) :
					r((int)(r * (ColorCount - 1))),
					g((int)(g * (ColorCount - 1))),
					b((int)(b * (ColorCount - 1))),
					a((int)(a * (ColorCount - 1)))
				{
					// empty
				}

				void assign(BYTE r, BYTE g, BYTE b, BYTE a = 0xFF) {
					this->r = r, this->g = g, this->b = b, this->a = a;
				}

				static inline BYTE ColorLimits(int value) {
					if ((value & ~0xFF) == 0)
						return value;
					else if (value < 0)
						return 0;
					else
						return 0xFF;
				}
				static inline Pixel FromIntegerColors(int r, int g, int b, int a = 0xFF) {
					return Pixel(ColorLimits(r), ColorLimits(g), ColorLimits(b), ColorLimits(a));
				}
				const uint32_t& toUInt() const {
					return *reinterpret_cast<const uint32_t*>(this);
				}
				uint32_t& toUInt() {
					return *reinterpret_cast<uint32_t*>(this);
				}
				bool operator==(const Pixel& other) const {
					return (toUInt() & 0xFFFFFF) == (other.toUInt() & 0xFFFFFF);
				}
				int GetBrightness() const {
					return ColorLimits((299 * (int)r + 587 * (int)g + 114 * (int)b) / 1000);
				}
			};
#pragma pack(pop)

			class Iterator {
			public:
				Iterator& operator++();
				Pixel& operator*() const;
				Pixel* operator->() const;
				bool operator!=(const Iterator& other) const;
				int get_x() const { return x; }
				int get_y() const { return image.get_real_y(y); }
			private:
				friend class Frame;
				int x;
				int y;
				Frame &image;
				Iterator(Frame& image, int x, int y);
				Iterator(Frame& image);
			};

			class ConstIterator {
			public:
				ConstIterator& operator++();
				const Pixel& operator*() const;
				const Pixel* operator->() const;
				bool operator!=(const ConstIterator& other) const;
				int get_x() const { return x; }
				int get_y() const { return image.get_real_y(y); }
			private:
				friend class Frame;
				int x;
				int y;
				const Frame &image;
				ConstIterator(const Frame& image, int x, int y);
				ConstIterator(const Frame& image);
			};

			friend class Iterator;
			friend class ConstIterator;

			Frame() :
				m_env(nullptr),
				m_width(0),
				m_height(0),
				m_pitch(0),
				m_read_ptr(nullptr),
				m_write_ptr(nullptr)
			{
				// empty
			}

			Frame(IScriptEnvironment* env, const VideoInfo& vi) :
				m_env(env),
				m_vi(vi),
				m_frame(env->NewVideoFrame(vi))
			{
				CheckRGB32();
				m_width = m_frame->GetRowSize() / sizeof(Pixel);
				m_height = m_frame->GetHeight();
				m_pitch = m_frame->GetPitch();
				m_read_ptr = m_frame->GetReadPtr();
				m_write_ptr = m_frame->GetWritePtr();
			}

			Frame(IScriptEnvironment* env, const VideoInfo& vi, const PVideoFrame frame) :
				m_env(env),
				m_vi(vi),
				m_frame(frame)
			{
				CheckRGB32();
				m_width = frame->GetRowSize() / sizeof(Pixel);
				m_height = frame->GetHeight();
				m_pitch = frame->GetPitch();
				m_read_ptr = frame->GetReadPtr();
				m_write_ptr = nullptr;
			}

			Frame(IScriptEnvironment* env, const PClip& clip, int framenum) :
				Frame(env, clip->GetVideoInfo(), clip->GetFrame(framenum, env))
			{
			}

			operator PVideoFrame() const { return m_frame; }
			int width() const { return m_width; }
			int height() const { return m_height; }
			BYTE* data() { return m_write_ptr; }
			const BYTE* cdata() const { return m_read_ptr; }

			int size() const {
				return m_frame->GetFrameBuffer()->GetDataSize();
			}

			bool empty() const {
				return m_env == nullptr;
			}

			const VideoInfo& GetVideoInfo() const { return m_vi; }

			Iterator begin() { return Iterator(*this, 0, 0); }
			Iterator end() { return Iterator(*this); }
			ConstIterator cbegin() const { return ConstIterator(*this, 0, 0); }
			ConstIterator cend() const { return ConstIterator(*this); }

			const Pixel& read(int x, int y) const {
				return read_row(y)[x];
			}
			Pixel& write(int x, int y) {
				return write_row(y)[x];
			}

			inline const Pixel* read_row(int y) const {
				return reinterpret_cast<const Pixel*>(m_read_ptr + get_real_y(y) * m_pitch);
			}
			inline Pixel* write_row(int y) {
				return reinterpret_cast<Pixel*>(m_write_ptr + get_real_y(y) * m_pitch);
			}

			const Pixel& read_raw(int x, int y) const {
				return reinterpret_cast<const Pixel*>(m_read_ptr + y * m_pitch)[x];
			}
			Pixel& write_raw(int x, int y) {
				return reinterpret_cast<Pixel*>(m_write_ptr + y * m_pitch)[x];
			}

			void Dump(const std::string& filename) const;
			void DumpModified(const std::string& filename);

			bool operator!=(const Frame& other) const;
			void EnableWriting();

		private:
			VideoInfo m_vi;
			IScriptEnvironment *m_env;
			PVideoFrame m_frame;
			int m_width;
			int m_height;
			int m_pitch;
			const BYTE *m_read_ptr;
			BYTE *m_write_ptr;

			int get_real_y(int offset_y) const {
				return (!m_vi.IsBFF()) ? offset_y : (m_vi.height - offset_y - 1);
			}
			void CheckRGB32() const {
				if (!m_vi.IsRGB32())
					m_env->ThrowError("Only RGB32 input supported");
			}
		};

		inline Frame::Iterator& Frame::Iterator::operator++() {
			++x;
			if (x == image.width()) {
				x = 0;
				++y;
			}
			return *this;
		}

		inline Frame::Pixel& Frame::Iterator::operator*() const {
			return reinterpret_cast<Pixel*>(image.data() + y * image.m_pitch)[x];
		}

		inline Frame::Pixel* Frame::Iterator::operator->() const {
			return &reinterpret_cast<Pixel*>(image.data() + y * image.m_pitch)[x];
		}

		inline bool Frame::Iterator::operator!=(const Iterator& other) const {
			return x != other.x || y != other.y;
		}

		inline Frame::Iterator::Iterator(Frame& image, int x, int y) :
			x(x), y(y), image(image)
		{
		}

		inline Frame::Iterator::Iterator(Frame& image) :
			x(0),
			y(image.height()),
			image(image)
		{
		}

		inline Frame::ConstIterator& Frame::ConstIterator::operator++() {
			++x;
			if (x == image.width()) {
				x = 0;
				++y;
			}
			return *this;
		}

		inline const Frame::Pixel& Frame::ConstIterator::operator*() const {
			return reinterpret_cast<const Pixel*>(image.cdata() + y * image.m_pitch)[x];
		}

		inline const Frame::Pixel* Frame::ConstIterator::operator->() const {
			return &reinterpret_cast<const Pixel*>(image.cdata() + y * image.m_pitch)[x];
		}

		inline bool Frame::ConstIterator::operator!=(const ConstIterator& other) const {
			return x != other.x || y != other.y;
		}

		inline Frame::ConstIterator::ConstIterator(const Frame& image, int x, int y) :
			x(x), y(y), image(image)
		{
		}

		inline Frame::ConstIterator::ConstIterator(const Frame& image) :
			x(0),
			y(image.height()),
			image(image)
		{
		}

	}
}
