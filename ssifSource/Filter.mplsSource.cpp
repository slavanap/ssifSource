// BD3D video decoding tool
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
#include "Filter.mplsSource.hpp"
#include "mplsReader.h"

#include <common.h>
#include <Tools.WinApi.hpp>

#define FILTER_NAME "mplsSource"

using namespace Tools::WinApi;

namespace Filter {

	void mplsSource::ChangeCurrentFile(int new_idx, IScriptEnvironment* env) {
		if (currentIndex == new_idx)
			return;
		currentClip = nullptr;
		printf("\n" FILTER_NAME "Changing current sequence from %s to %s\n",
			(currentIndex < 0) ? "<none>" : fileNames[currentIndex].c_str(),
			fileNames[new_idx].c_str());

		AVSValue args[9] = { fileNames[new_idx].c_str(), frameOffsets[new_idx + 1] - frameOffsets[new_idx] };
		for (int i = 2; i < 9; ++i)
			args[i] = pluginParams[i];
		if (!args[5].Defined())
			args[5] = flagSwapViews;
		currentClip = (env->Invoke("ssifSource", AVSValue(args, 9))).AsClip();
		currentIndex = new_idx;
	}

	PVideoFrame mplsSource::GetFrame(int n, IScriptEnvironment* env) {
		int idx = currentIndex;
		while (n >= frameOffsets[idx + 1])
			++idx;
		while (frameOffsets[idx] > n)
			--idx;
		ChangeCurrentFile(idx, env);
		return currentClip->GetFrame(n - frameOffsets[idx], env);
	}

	mplsSource::mplsSource(IScriptEnvironment* env, AVSValue args) {
		// Note, vi will be initialized at the end of constructor

		// copy plug-in arguments for later usage
		pluginParamValues.reset(new AVSValue[args.ArraySize()]);
		for (int i = 0; i < args.ArraySize(); ++i)
			pluginParamValues[i] = args[i];
		pluginParams = AVSValue(pluginParamValues.get(), args.ArraySize());

		// currently loaded file is undefined
		currentIndex = -1;

		flagMVC = false;

		std::string mplsFilename = args[0].AsString();
		std::string mplsDir = ExtractFileDir(mplsFilename);
		ssifPath = args[1].Defined() ? args[1].AsString() : (mplsDir + "..\\STREAM\\");
		if (IsDirectoryExists((ssifPath + "SSIF").c_str())) {
			flagMVC = true;
			ssifPath += "SSIF\\";
		}

		// parse playlist
		mpls_file_t mpls_file;
		playlist_t playlist_base;
		try {
			mpls_file = init_mpls(&mplsFilename[0]);
			playlist_base = create_playlist_t();
			parse_stream_clips(&mpls_file, &playlist_base);
			print_stream_clips_header(&playlist_base);
			print_stream_clips(&playlist_base);
		}
		catch (std::exception& err) {
			env->ThrowError("%s", err.what());
		}

		// swap views auto-detection
		flagSwapViews = TEST(mpls_file.data[0x38], 0x10);
		printf("%s: Swap views flag value is %s\n", FILTER_NAME, flagSwapViews ? "true" : "false");

		playlist_t *playlist = &playlist_base;
		stream_clip_t *clip = playlist->stream_clip_list.first;
		int num_frames = 0;
		while (clip != nullptr) {
			int currect_framecount = (int)((double)clip->raw_duration * 24 / (45 * 1001) + 0.5);
			printf("%s: adding file %s with %d frames to sequences list.\n", FILTER_NAME, clip->filename, currect_framecount);

			std::string filename = clip->filename;
			filename.erase(filename.length() - 4);
			filename = ssifPath + filename + (flagMVC ? "SSIF" : "M2TS");
			fileNames.push_back(filename);

			frameOffsets.push_back(num_frames);
			num_frames += currect_framecount;
			clip = clip->next;
		}
		frameOffsets.push_back(num_frames);
		free_playlist_members(&playlist_base);
		free_mpls_file_members(&mpls_file);

		ChangeCurrentFile(0, env);
		vi = currentClip->GetVideoInfo();
		vi.num_frames = *frameOffsets.rbegin();
	}

	mplsSource::~mplsSource() {
		currentClip = nullptr;
	}

	AvsParams mplsSource::CreateParams =
		"[mpls_file]s"
		"[ssif_path]s"
		"[left_view]b"
		"[right_view]b"
		"[horizontal_stack]b"
		"[swap_views]b"
		"[intel_params]s"
		"[debug]b"
		"[use_ldecod]b";

	AVSValue mplsSource::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
		try {
			return new mplsSource(env, args);
		}
		catch (std::exception& ex) {
			env->ThrowError("ERROR in %s : %s", FILTER_NAME, ex.what());
		}
		return AVSValue();
	}

}

extern "C" void die(const char* filename, int line_number, const char * format, ...) {
	va_list vargs;
	va_start(vargs, format);
	char buffer[256];
	vsprintf_s(buffer, sizeof(buffer), format, vargs);
	throw std::runtime_error(buffer);
}
