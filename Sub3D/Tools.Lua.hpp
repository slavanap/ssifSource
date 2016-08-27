//
// 3D subtitle rendering tool
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

#include <Tools.AviSynth.hpp>		// only for AvsParams

struct lua_State;

namespace Tools {
	namespace Lua {

		class Script {
		public:
			typedef std::vector<std::pair<int, double>> depth_list_t;

			Script(const std::string& xmlfile, const std::string& srtfile, const VideoInfo& vi);
			~Script();

			void CalculateForFrame(int framenum, const depth_list_t& list);
			bool CalculateForSubtitle(int startFramenum, int lengthInFrames, int& depth);

			static void SetLuaFile(const std::string& filename) {
				m_luafile = filename;
			}

		private:
			static std::string m_luafile;
			lua_State *L;
			int ref_CalcForFrame, ref_CalcForSub, ref_OnFinish;
			const depth_list_t *m_depth_list;

			bool pcall(int n_args, int n_results, int error_handler = 1);
			void OnFinish();
			static int luaCalcForFrameRaw(lua_State* L);
			static int luaGetFrameRawData(lua_State* L);
		};

		extern AvsParams SetLuaFileParams;
		AVSValue SetLuaFile(AVSValue args, void* user_data, IScriptEnvironment* env);

	}
}
