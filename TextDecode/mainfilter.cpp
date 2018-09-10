#include "stdafx.h"
#include "mainfilter.h"

#include <iostream>

using Tools::AviSynth::Frame;

constexpr int use_x = 72;
constexpr int use_y = 22;
constexpr int cmp_w = 20;
constexpr int cmp_h = 40;

constexpr int non_stable_frames = 8;
constexpr int diff_threshold = 32000;

uint32_t diff(const Frame& a, uint32_t a_x, uint32_t a_y, const Frame& b, uint32_t b_x, uint32_t b_y) {
	uint32_t ret = 0;
	for (int y = 0; y < cmp_h; ++y) {
		for (int x = 0; x < cmp_w; ++x) {
			auto pa = a.read(x + a_x, y + a_y);
			auto pb = b.read(x + b_x, y + b_y);
			ret += (pa.r - pb.r) * (pa.r - pb.r);
		}
	}
	return ret;
}

constexpr int use_all = use_x * use_y;
const char* const char_map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
bool first_init = true;

TextDecode::TextDecode(IScriptEnvironment* env, PClip input, PClip symbols, int detect_offset, int trusted_lines) :
	GenericVideoFilter(Tools::AviSynth::ConvertToRGB32(env, input)),
	_file("result.txt"), _detect_offset(detect_offset), _mismatch_frames(0), _prev_output_frame(0), _trusted_lines(trusted_lines)
{
	if (first_init) {
		first_init = false;
#pragma warning(push)
#pragma warning(disable: 4996)
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		freopen("CONOUT$", "wb", stderr);
#pragma warning(pop)
	}

	_symbols = Frame(env, Tools::AviSynth::ConvertToRGB32(env, symbols), 0);
}

TextDecode::~TextDecode() {
	for (size_t i = 0; i < _lines.size(); ++i)
		_file << (_detect_offset + _prev_output_frame) << '\t' << _lines[i] << std::endl;
}

bool strcompare(const std::string& a, const std::string& b) {
	if (a.size() != b.size())
		return false;
	size_t half = a.size() / 2;
	if (!strncmp(&a[0], &b[0], half))
		return true;
	if (!strncmp(&a[half + 1], &b[half + 1], a.size() - half))
		return true;
	return false;
}

PVideoFrame WINAPI TextDecode::GetFrame(int n, IScriptEnvironment* env) {
	Frame frame(env, child, n);

	uint32_t max_error = 0;
	int max_crop_idx = 0;
	char max_symb = ' ';
	std::vector<std::string> new_lines;
	for (int crop_y = 0; crop_y < _trusted_lines; ++crop_y) {
		std::string line(use_x, 0);
		for (int crop_x = 0; crop_x < use_x; ++crop_x) {
			uint32_t val = (std::numeric_limits<uint32_t>::max)();
			size_t pos = 0;
			for (size_t symb_idx = 0; symb_idx < 65; ++symb_idx) {
				uint32_t cur = diff(frame, crop_x * cmp_w + 271, crop_y * cmp_h, _symbols, (symb_idx % 8) * cmp_w, (symb_idx / 8) * cmp_h);
				if (cur < val) {
					pos = symb_idx;
					val = cur;
				}
			}
			line[crop_x] = char_map[pos];
			if (val > max_error) {
				max_crop_idx = crop_x + crop_y * use_x;
				max_symb = char_map[pos];
				max_error = val;
				if (max_error > diff_threshold)
					goto after_detection;
			}
		}
		new_lines.push_back(std::move(line));
	}

after_detection:
	std::cout 
		<< n
		<< '\t' << (n + _detect_offset)
		<< '\t'	<< max_error
		<< '\t' << (max_crop_idx / use_x)
		<< ' ' << (max_crop_idx % use_x)
		<< ' ' << max_symb
		<< '\t' << _lines.size()
		<< '\t' << new_lines.size()
		<< std::endl;
	
	if (new_lines.size() >= 3) {
		if (_lines.size() == 0) {
			_lines = new_lines;
		}
		else if (_lines[0] != new_lines[0] || new_lines.size() > _lines.size()) {
			// check for dups
			for (size_t i = 1; i < new_lines.size(); ++i)
				if (strcompare(new_lines[i - 1], new_lines[i]))
					goto next;
			for (size_t i = 2; i < new_lines.size(); ++i)
				if (strcompare(new_lines[i - 2], new_lines[i]))
					goto next;
			if (++_mismatch_frames >= non_stable_frames) {
				bool match = false;
				size_t match_line = 0;
				for (size_t i = 0; i < _lines.size(); ++i) {
					if (_lines[i] == new_lines[0]) {
						match = true;
						match_line = i;
						break;
					}
				}
				if (match) {
					if (match_line > 0) {
						for (size_t j = 0; j < match_line; ++j) {
							_file << (_detect_offset + _prev_output_frame) << '\t' << _lines[j] << std::endl;
							std::cout << _prev_output_frame << ' ' << (_detect_offset + _prev_output_frame) << '\t' << _lines[j] << std::endl;
						}
						_mismatch_frames = 0;
						_prev_output_frame = n;
					}
					_lines.erase(_lines.begin(), _lines.begin() + match_line);
					if (new_lines.size() >= _lines.size()) {
						_lines = new_lines;
					}
					else {
						for (size_t i = 0; i < new_lines.size() - 2; ++i)
							if (_lines[i] != new_lines[i])
								env->ThrowError("Bad match!");
					}
				}
				else if (_mismatch_frames > 100) {
					std::stringstream ss;
					ss << "Invalid match at frame " << n;
					env->ThrowError(ss.str().c_str());
				}
			}
		next:;
		}
	}
	return frame;
}



AvsParams TextDecode::CreateParams = "ccii";

AVSValue __cdecl TextDecode::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new TextDecode(env,
		args[0].AsClip(),
		args[1].AsClip(),
		args[2].AsInt(0),
		args[3].AsInt(10)		// 13 in case of fast shift
	);
}
