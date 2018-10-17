//
// Match 2 videos by frames
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
#include <cstdint>
#include <memory>
#include <vector>

#include "Tools.AviSynth.Frame.hpp"

namespace Filter {

	template<typename T>
	class Correlation {
	public:
		Correlation(const std::vector<T>& pattern, T maxvalue, T threshold = 0) :
			m_threshold(threshold),
			m_maxvalue(maxvalue),
			m_inited(false),
			m_last_pos(), m_best_pos(), m_last_error(), m_best_error()
		{
			if (pattern.empty())
				throw std::runtime_error("Pattern could not be empty");
			m_pattern_size = pattern.size();
			InitHistogram(m_pattern_histogram, pattern);
			m_init.reserve(m_pattern_size);
		}
		
		bool Next(T nextval) {
			if (!m_inited) {
				m_init.push_back(nextval);
				if (m_init.size() == m_pattern_size) {
					InitHistogram(m_init_histogram, m_init);
					int64_t error = 0;
					for (size_t i = 0; i <= m_maxvalue; ++i)
						error += sqr((int64_t)m_init_histogram[i] - m_pattern_histogram[i]);
					m_best_error = m_last_error = error;
					m_best_pos = m_last_pos = 0;
					m_inited = true;
					return m_best_error <= m_threshold;
				}
				return false;
			}
			//if (m_best_error <= 0)
			//	return true;
			size_t realpos = m_last_pos % m_pattern_size;
			T prev_val = m_init[realpos];
			m_init[realpos] = nextval;
			m_last_error -= sqr((int64_t)m_init_histogram[prev_val] - m_pattern_histogram[prev_val]) +
				sqr((int64_t)m_init_histogram[nextval] - m_pattern_histogram[nextval]);
			--m_init_histogram[prev_val], ++m_init_histogram[nextval];
			m_last_error += sqr((int64_t)m_init_histogram[prev_val] - m_pattern_histogram[prev_val]) +
				sqr((int64_t)m_init_histogram[nextval] - m_pattern_histogram[nextval]);
			if (m_last_error < m_best_error) {
				m_best_error = m_last_error;
				m_best_pos = m_last_pos;
			}
			++m_last_pos;
			return m_best_error <= m_threshold;
		}

		unsigned int GetMatch() const {
			return m_best_pos;
		}

		T GetThreshold() const {
			return m_threshold;
		}

		bool IsMatchFound() const {
			return m_best_error <= m_threshold;
		}

	private:
		void InitHistogram(std::vector<int>& hist, const std::vector<T>& values) {
			hist.clear();
			hist.resize(m_maxvalue+1, 0);
			for (T v : values)
				++hist[v];
		}

		T m_threshold, m_maxvalue;
		int64_t m_best_error, m_last_error;
		size_t m_pattern_size, m_best_pos, m_last_pos;
		bool m_inited;
		std::vector<T> m_init;
		std::vector<int> m_pattern_histogram, m_init_histogram;
	};

	class VideoCorrelation : public GenericVideoFilter {
	public:
		using SigValue = uint32_t;
		using Signature = std::vector<SigValue>;
		static Signature::value_type ComputeForFrame(const Tools::AviSynth::Frame& frame);

		static AvsParams CreateParams;
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

		static AvsParams PreprocessParams;
		static AVSValue __cdecl Preprocess(AVSValue args, void* user_data, IScriptEnvironment* env);
		static Signature Preprocess(IScriptEnvironment* env, PClip clip);

		static AvsParams GetShiftParams;
		static AVSValue __cdecl GetShift(AVSValue args, void* user_data, IScriptEnvironment* env);
		static int GetShift(IScriptEnvironment* env, PClip overlay, const Signature& input_sig);

		VideoCorrelation(IScriptEnvironment* env, PClip input, PClip overlay,
			int shift = -1, const Signature& input_sig = Signature(), SigValue threshold = 0);

		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;
		void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override;
		bool WINAPI GetParity(int n) override;

	private:
		PClip m_overlay;
		int m_shift;
		VideoInfo m_overlay_vi;
		int m_overlay_sig_shift;
		std::unique_ptr<Correlation<SigValue>> m_correlation;
	};

}
