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

#include "stdafx.h"
#include "Tools.AviSynth.Frame.hpp"

namespace Tools {

	/** Correlation detection automate.\ Class that detects correlation between histograms
		with specified pattern histogram.
		It searches square error minimum (computed for number of elements with the same value in histograms)
		and reports if it's less than `threshold`.
		See VideoCorrelation.Test.cpp for the data sample (note, order of values does not matter).
	*/
	template<typename T>
	class Correlation {
	public:
		/** Initializes the automate with \a pattern to detect, \a maxvalue that is the maximum possible
			value in the pattern and a \a threshold that determines when to report a match
		*/
		Correlation(const std::vector<T>& pattern, size_t maxvalue, T threshold = 0) :
			m_threshold(threshold),
			m_maxvalue(maxvalue),
			m_inited(false),
			m_last_pos(), m_best_pos(), m_last_error(), m_best_error((std::numeric_limits<T>::max)())
		{
			if (pattern.empty())
				throw std::runtime_error("Pattern could not be empty");
			m_pattern_size = pattern.size();
			InitHistogram(m_pattern_histogram, pattern);
			m_init.reserve(m_pattern_size);
		}

		/// Gives \a nextval (next value) to the automate that detects matches
		bool Next(const T& nextval) {
			if (!m_inited) {
				// We can't report anything unless we get at least that much elements as the pattern has.
				m_init.push_back(nextval);
				if (m_init.size() == m_pattern_size) {
					// When we've got enough items than perform the init.
					InitHistogram(m_init_histogram, m_init);
					int64_t error = 0;
					for (size_t i = 0; i <= m_maxvalue; ++i)
						error += sqr((int64_t)m_init_histogram[i] - m_pattern_histogram[i]); // MSE-like error function
					m_best_error = m_last_error = error;
					m_best_pos = m_last_pos = 0;
					m_inited = true;
					return m_best_error <= m_threshold;
				}
				return false;
			}
			size_t realpos = m_last_pos % m_pattern_size;
			T prev_val = m_init[realpos];
			m_init[realpos] = nextval;
			// fast formulas for error value recomputation (histogram window)
			m_last_error -= sqr((int64_t)m_init_histogram[prev_val] - m_pattern_histogram[prev_val]) +
				sqr((int64_t)m_init_histogram[nextval] - m_pattern_histogram[nextval]);
			--m_init_histogram[prev_val], ++m_init_histogram[nextval];
			m_last_error += sqr((int64_t)m_init_histogram[prev_val] - m_pattern_histogram[prev_val]) +
				sqr((int64_t)m_init_histogram[nextval] - m_pattern_histogram[nextval]);
			// check whether we found better match
			if (m_last_error < m_best_error) {
				m_best_error = m_last_error;
				m_best_pos = m_last_pos;
			}
			++m_last_pos;
			return m_best_error <= m_threshold;
		}

		/// After Next() returned true, GetMatch function returns the match position within the input stream
		unsigned int GetMatch() const {
			return m_best_pos;
		}

		/// Get the threshold that the automate was inited with.
		T GetThreshold() const {
			return m_threshold;
		}

		/// Returns true, if match has been ever found before
		bool IsMatchFound() const {
			return m_best_error <= m_threshold;
		}

	private:
		void InitHistogram(std::vector<int>& hist, const std::vector<T>& values) {
			hist.clear();
			hist.resize(m_maxvalue + 1, 0);
			for (const T& v : values)
				++hist[v];
		}

		T m_threshold;
		size_t m_maxvalue;
		int64_t m_best_error, m_last_error;
		size_t m_pattern_size, m_best_pos, m_last_pos;
		bool m_inited;
		std::vector<T> m_init;
		std::vector<int> m_pattern_histogram, m_init_histogram;
	};

}

namespace Filter {

	/** VideoCorrelation filter for AviSynth.\ Glues 2 videos where overlay is a subset of the input.
		"Overlay" fragment is expected to appear somewhere in the "input" sequence.
		At the beginning the filter outputs "input" sequence (audio+video).
		When it finds the correlation match where the "input" sequence matches the start of the "overlay" sequence,
		it switches to output "overlay" sequence (audio+video) until "overlay" is over.
		The Preprocess function is intended to speed up the filter launch (precomputes signature for "input" sequence).
		The class is useful when you have one video with muted parts ("input") and a stream recording started
		somewhere after the place where the input video starts, but that contains full audio. The stiching result
		will contain as much audio content as possible.
	*/
	class VideoCorrelation : public GenericVideoFilter {
	public:
		using SigValue = uint32_t;				 ///< Metric value for video frame aka "frame descriptor"
		using Signature = std::vector<SigValue>; ///< Pattern for videosequence to match, rather short.

		/// Computes metric value for single \a frame video frame
		static Signature::value_type ComputeForFrame(const Tools::AviSynth::Frame& frame);

		/// Initializes the filter
		VideoCorrelation(IScriptEnvironment* env, PClip input, PClip overlay,
			int shift = -1, const Signature& input_sig = Signature(), SigValue threshold = 0);
		static AvsParams CreateParams;	///< AviSynth function parameters descriptor
		static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env); ///< AviSynth function that creates VideoCorrelation filter

		/** Creates video signature for video fragment. May be used in order to speed up the filter initialization,
			and prevent "overlay" sequence rewind, or to create signature of "input" sequence and detect
			shift of "overlay" within the "input" in frames with \a GetShift method
		*/
		static Signature Preprocess(IScriptEnvironment* env, PClip clip);
		static AvsParams PreprocessParams;
		static AVSValue __cdecl Preprocess(AVSValue args, void* user_data, IScriptEnvironment* env);

		/** Internally creates signature of \a overlay video sequence and tries to find its correlation within
			\a input_sig precomputed signature for the whole input clip.
		*/
		static int GetShift(IScriptEnvironment* env, PClip overlay, const Signature& input_sig);
		static AvsParams GetShiftParams;
		static AVSValue __cdecl GetShift(AVSValue args, void* user_data, IScriptEnvironment* env);

		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;
		void WINAPI GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) override;
		bool WINAPI GetParity(int n) override;

	private:
		PClip m_overlay;
		int m_shift;
		VideoInfo m_overlay_vi;
		int m_overlay_sig_shift;
		std::unique_ptr<Tools::Correlation<SigValue>> m_correlation;
	};

}
