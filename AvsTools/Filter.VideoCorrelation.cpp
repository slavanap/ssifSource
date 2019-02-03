#include "stdafx.h"
#include <iterator>
#include <fstream>

#include "Tools.AviSynth.Frame.hpp"
#include "Filter.VideoCorrelation.hpp"

namespace Filter {

	// API & Tools

	/// Writes \a vs signature to file stream \a out
	std::ostream& operator<<(std::ostream& out, const VideoCorrelation::Signature& vs) {
		out << vs.size() << std::endl;
		for (auto it = vs.begin(); it != vs.end(); ++it)
			out << *it << " ";
		return out;
	}

	/// Reads \a vs signature from the file stream \a in
	std::istream& operator>>(std::istream& in, VideoCorrelation::Signature& vs) {
		size_t temp;
		if (!(in >> temp))
			goto error;
		vs.resize(temp);
		for (size_t i = 0; i < temp; ++i) {
			if (!(in >> vs[i]))
				goto error;
		}
		return in;
	error:
		throw std::runtime_error("Invalid array in file");
	}

	AvsParams VideoCorrelation::CreateParams = "[input]c[overlay]c[shift]i[threshold]i[preprocess_input]s";
	AVSValue VideoCorrelation::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		PClip input = args[0].AsClip();
		PClip overlay = args[1].AsClip();
		int shift = args[2].AsInt(-1);
		int threshold = args[3].AsInt(0);
		const char* preprocess = args[4].AsString(nullptr);
		try {
			Signature sig;
			if (preprocess != nullptr) {
				std::ifstream f(preprocess);
				if (!f.good())
					throw std::runtime_error("Can't open file");
				f >> sig;
			}
			return new VideoCorrelation(env, input, overlay, shift, sig, threshold);
		}
		catch (std::exception& e) {
			env->ThrowError(e.what());
			return AVSValue();
		}
	}

	AvsParams VideoCorrelation::PreprocessParams = "c[output]s";
	AVSValue VideoCorrelation::Preprocess(AVSValue args, void* user_data, IScriptEnvironment* env) {
		try {
			std::ofstream f(args[1].AsString());
			if (!f.good())
				throw std::runtime_error("Can't create file");
			auto sig = Preprocess(env, args[0].AsClip());
			f << sig;
			f.close();
			return AVSValue();
		}
		catch (std::exception& e) {
			env->ThrowError("%s", e.what());
			return AVSValue();
		}
	}

	AvsParams VideoCorrelation::GetShiftParams = "[overlay]c[input_sig]s";
	AVSValue VideoCorrelation::GetShift(AVSValue args, void* user_data, IScriptEnvironment* env) {
		try {
			std::ifstream f(args[1].AsString());
			if (!f.good())
				throw std::runtime_error("Can't open file");
			Signature input_sig;
			f >> input_sig;
			return GetShift(env, args[0].AsClip(), input_sig);
		}
		catch (std::exception& e) {
			env->ThrowError("%s", e.what());
			return AVSValue();
		}
	}



	// Algorithms

	using Tools::AviSynth::Frame;

	/** We use floor(sum_i(pixel_i.red^2 + pixel_i.green^2 + pixel_i.blue^2) * 100 / #i) as a signature value for a frame.
		Here's a maximum possible value for that function (assume 0 < pixel_i.* < 256).
	*/
	constexpr size_t MaxSigValue = (size_t)((Frame::ColorCount - 1) * (Frame::ColorCount - 1) * 3) * 100;

	VideoCorrelation::Signature::value_type VideoCorrelation::ComputeForFrame(const Frame& frame) {
		uint64_t result = 0;
		for (int j = 0; j < frame.height(); ++j) {
			const Frame::Pixel *row = frame.read_row(j);
			for (int k = 0; k < frame.width(); ++k) {
				const Frame::Pixel &px = row[k];
				result += (uint32_t)px.r*px.r + (uint32_t)px.g*px.g + (uint32_t)px.b*px.b;
			}
		}
		result = result * 100 / (frame.width() * frame.height());
		return static_cast<SigValue>(result);
	}

	VideoCorrelation::Signature VideoCorrelation::Preprocess(IScriptEnvironment* env, PClip clip) {
		clip = Tools::AviSynth::ConvertToRGB32(env, clip);
		int len = clip->GetVideoInfo().num_frames;
		Signature result;
		result.reserve(len);
		for (int i = 0; i < len; ++i)
			result.push_back(ComputeForFrame(Frame(env, clip, i)));
		return result;
	}

	int VideoCorrelation::GetShift(IScriptEnvironment* env, PClip overlay, const Signature& input_sig) {
		if ((int)input_sig.size() <= overlay->GetVideoInfo().num_frames)
			throw std::exception("Overlay must me shorter than input");
		Signature overlay_sig = Preprocess(env, overlay);
		Tools::Correlation<SigValue> c(overlay_sig, MaxSigValue);
		for (SigValue v : input_sig)
			if (c.Next(v))
				break;
		return c.GetMatch();
	}



	// Overlay implementation

	constexpr int OverlaySigShiftFrames = 1200;
	constexpr int OverlaySigLength = 1000;

	VideoCorrelation::VideoCorrelation(IScriptEnvironment* env, PClip input, PClip overlay,
		int shift, const Signature& input_sig, Signature::value_type threshold)
		:
		GenericVideoFilter(Tools::AviSynth::ConvertToRGB32(env, input)),
		m_overlay(Tools::AviSynth::ConvertToRGB32(env, overlay)),
		m_shift(shift)
	{
		m_overlay_vi = m_overlay->GetVideoInfo();

		if (vi.width != m_overlay_vi.width || vi.height != m_overlay_vi.height || vi.pixel_type != m_overlay_vi.pixel_type)
			env->ThrowError("Overlay and input properties must be equal");

		if (vi.num_frames <= m_overlay_vi.num_frames)
			env->ThrowError("Overlay must be shorter than input");

		// compute overlay signature that has "overlay_sig_len" length
		int overlay_sig_len = std::min(m_overlay_vi.num_frames, OverlaySigLength);

		// compute overlay signature starting from "m_overlay_sig_shift" frame number.
		m_overlay_sig_shift = std::min(m_overlay_vi.num_frames - overlay_sig_len, OverlaySigShiftFrames);
		if (m_overlay_sig_shift < OverlaySigShiftFrames)
			fprintf(stderr, "WARNING: overlay is too short. Consider increasing its length to improve matching quality\n");

		if (m_shift < 0) {
			for (int i = 0; i < m_overlay_sig_shift; ++i)
				m_overlay->GetFrame(i, env);
			Signature overlay_sig;
			overlay_sig.reserve(overlay_sig_len);
			for (int i = m_overlay_sig_shift; i < m_overlay_sig_shift + overlay_sig_len; ++i)
				overlay_sig.push_back(ComputeForFrame(Frame(env, m_overlay, i)));
			m_correlation = std::make_unique<Tools::Correlation<SigValue>>(overlay_sig, MaxSigValue, threshold);
			if (!input_sig.empty()) {
				for (SigValue v : input_sig) {
					m_correlation->Next(v);
				}
				if (m_correlation->IsMatchFound()) {
					m_shift = m_correlation->GetMatch() - m_overlay_sig_shift;
				}
				else {
					m_shift = vi.num_frames;
					fprintf(stderr, "WARNING: no match found with specified threshold\n");
				}
			}
			for (int i = 0; i < m_overlay_sig_shift; ++i)
				m_overlay->GetFrame(i, env);
		}
	}

	PVideoFrame VideoCorrelation::GetFrame(int n, IScriptEnvironment* env) {
		// if the shift value hasn't been found yet
		if (m_shift < 0) {
			// keep trying to find the correlation
			if (m_correlation->Next(ComputeForFrame(Frame(env, child, n))))
				m_shift = m_correlation->GetMatch() - m_overlay_sig_shift;
		}
		if (0 <= m_shift && m_shift < vi.num_frames) {
			// if the shift value has been found, render the overlay accordingly
			if (m_shift <= n && n < m_shift + m_overlay_vi.num_frames)
				return m_overlay->GetFrame(n - m_shift, env);
		}
		// if shift hasn't been found yet, or if the overlay is short that it lies inside the input, then render the input.
		return child->GetFrame(n, env);
	}

	void VideoCorrelation::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
		// switch audio the same way as we switch video for the overlay
		if (0 <= m_shift && m_shift < vi.num_frames) {
			auto audio_start = vi.AudioSamplesFromFrames(m_shift);
			auto audio_end = vi.AudioSamplesFromFrames(m_shift + m_overlay_vi.num_frames);
			if (audio_start <= start && start < audio_end - count) {
				m_overlay->GetAudio(buf, start - audio_start, count, env);
				return;
			}
		}
		child->GetAudio(buf, start, count, env);
	}

	bool VideoCorrelation::GetParity(int n) {
		// same goes for the parity
		if (0 <= m_shift && m_shift < vi.num_frames) {
			if (m_shift <= n && n < m_shift + m_overlay_vi.num_frames)
				return m_overlay->GetParity(n - m_shift);
		}
		return child->GetParity(n);
	}

}
