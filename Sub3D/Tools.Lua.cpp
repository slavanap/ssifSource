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
#include "Tools.Lua.hpp"
#include <Tools.WinApi.hpp>
#include <lua/src/lua.hpp>

using namespace Tools::WinApi;

namespace Tools {
	namespace Lua {

		static std::unordered_map<lua_State*, Script*> m_luaStates;

		static int lua_error_handler(lua_State* L) {
			lua_pushvalue(L, 1);
			luaL_traceback(L, L, "", 2);
			lua_pushstring(L, "\n");
			lua_concat(L, 3);
			return 1;
		}

		bool Script::pcall(int n_args, int n_results, int error_handler) {
			int error = lua_pcall(L, n_args, n_results, error_handler);
			if (error) {
				fprintf(stderr, "err_code %d, %s", error, lua_tostring(L, -1));
				lua_pop(L, 1);
				return false;
			}
			return true;
		}



		std::string Script::m_luafile = "sub3d.lua";

		Script::Script(const std::string& xmlfile, const std::string& srtfile, const VideoInfo& vi) :
			m_depth_list(nullptr)
		{
			std::string error_message;

			L = luaL_newstate();
			lua_pushcfunction(L, lua_error_handler);
			luaL_openlibs(L);

			lua_getglobal(L, "package");
			int package = lua_gettop(L);
			lua_pushstring(L, "?.lua;");								// ?.lua;
			lua_pushstring(L, tostring(szLibraryPath).c_str());			// <dll_dir>\ 
			lua_pushvalue(L, -2);										// ?.lua;
			size_t pos = m_luafile.find_last_of('\\');
			if (pos != std::string::npos)
				lua_pushlstring(L, m_luafile.data(), pos + 1);			// <luafile_dir>\ 
			else
				lua_pushstring(L, ".\\");								// .\ 
			lua_pushvalue(L, -2);										// ?.lua;
			lua_getfield(L, package, "path");							// <prev_value>
			lua_concat(L, 6);
			lua_setfield(L, -2, "path");
			lua_pop(L, 1);

			if (!xmlfile.empty()) {
				lua_pushlstring(L, xmlfile.data(), xmlfile.size());
				lua_setglobal(L, "XML_FILE");
			}
			if (!srtfile.empty()) {
				lua_pushlstring(L, srtfile.data(), srtfile.size());
				lua_setglobal(L, "SRT_FILE");
			}
			lua_pushinteger(L, vi.num_frames);
			lua_setglobal(L, "FRAMECOUNT");
			lua_pushinteger(L, vi.fps_numerator);
			lua_setglobal(L, "FPS_NUMERATOR");
			lua_pushinteger(L, vi.fps_denominator);
			lua_setglobal(L, "FPS_DENOMINATOR");
			lua_pushnumber(L, (lua_Number)vi.fps_numerator / vi.fps_denominator);
			lua_setglobal(L, "FRAMERATE");

			m_luaStates[L] = this;
			lua_pushcfunction(L, luaCalcForFrameRaw);
			lua_setglobal(L, "CalcForFrameRaw");
			lua_pushcfunction(L, luaGetFrameRawData);
			lua_setglobal(L, "GetFrameRawData");

			lua_getglobal(L, "require");
			size_t dotpos = m_luafile.find_last_of('.');
			if (dotpos == std::string::npos)
				dotpos = m_luafile.size();
			if (pos != std::string::npos) {
				dotpos = std::max(dotpos, pos);
				lua_pushlstring(L, m_luafile.c_str() + (pos + 1), dotpos - pos);
			}
			else
				lua_pushlstring(L, m_luafile.c_str(), dotpos);
			if (!pcall(1, 0)) {
				error_message = "Failed to load '" + m_luafile + "'";
				goto error;
			}

			lua_getglobal(L, "CalcForFrame");
			ref_CalcForFrame = luaL_ref(L, LUA_REGISTRYINDEX);
			lua_getglobal(L, "CalcForSub");
			ref_CalcForSub = luaL_ref(L, LUA_REGISTRYINDEX);
			lua_getglobal(L, "OnFinish");
			ref_OnFinish = luaL_ref(L, LUA_REGISTRYINDEX);

			if (ref_CalcForFrame == LUA_REFNIL || ref_CalcForSub == LUA_REFNIL) {
				error_message = "required lua functions is not defined in the .lua file";
				goto error;
			}

			return;

		error:
			m_luaStates.erase(L);
			lua_close(L);
			throw std::runtime_error(error_message);
		}

		Script::~Script() {
			OnFinish();
			m_luaStates.erase(L);
			lua_close(L);
		}

		int Script::luaCalcForFrameRaw(lua_State* L) {
			Script *self = m_luaStates[L];
			if (!self->m_depth_list)
				luaL_error(L, "Unexpected. You should call CalcForFrameRaw only in borders of CalcForFrame function!");
			const depth_list_t &list = *self->m_depth_list;

			auto conf_threshold = luaL_optnumber(L, 1, 0);
			int count = 0;
			int min_depth = std::numeric_limits<int>::max(), max_depth = std::numeric_limits<int>::min(), sum_depth = 0;
			double min_conf = 1, max_conf = 0, sum_conf = 0;
			for (auto it = list.begin(); it != list.end(); ++it) {
				if (it->second >= conf_threshold) {
					if (it->second < min_conf)
						min_conf = it->second;
					if (it->second > max_conf)
						max_conf = it->second;
					sum_conf += it->second;
					if (it->first < min_depth)
						min_depth = it->first;
					if (it->first > max_depth)
						max_depth = it->first;
					sum_depth += it->first;
					count++;
				}
			}

			if (count == 0)
				return 0;

			lua_pushinteger(L, min_depth);
			lua_pushinteger(L, max_depth);
			lua_pushnumber(L, (double)sum_depth / count);
			lua_pushnumber(L, min_conf);
			lua_pushnumber(L, max_conf);
			lua_pushnumber(L, sum_conf / count);
			return 6;
		}

		int Script::luaGetFrameRawData(lua_State* L) {
			Script *self = m_luaStates[L];
			if (!self->m_depth_list)
				luaL_error(L, "Unexpected situation. You should call CalcForFrameRaw only in borders of CalcForFrame function!");
			const depth_list_t &list = *self->m_depth_list;
			std::map<depth_list_t::value_type, int> counting;
			for (auto it = list.begin(); it != list.end(); ++it) {
				auto sit = counting.find(*it);
				if (sit == counting.end())
					counting[*it] = 1;
				else
					sit->second++;
			}
			lua_pushstring(L, "depth");
			lua_pushstring(L, "conf");
			lua_pushstring(L, "count");
			lua_createtable(L, list.size(), 0);
			int top = lua_gettop(L);
			int idx = 0;
			for (auto it = counting.begin(); it != counting.end(); ++it) {
				lua_createtable(L, 0, 3);
				lua_pushvalue(L, top - 3);	// depth
				lua_pushinteger(L, it->first.first);
				lua_rawset(L, -3);
				lua_pushvalue(L, top - 2);	// conf
				lua_pushnumber(L, it->first.second);
				lua_rawset(L, -3);
				lua_pushvalue(L, top - 1);	// count
				lua_pushinteger(L, it->second);
				lua_rawset(L, -3);
				lua_rawseti(L, top, ++idx);
			}
			return 1;
		}

		void Script::CalculateForFrame(int framenum, const depth_list_t& list) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref_CalcForFrame);
			lua_pushinteger(L, framenum);
			m_depth_list = &list;
			bool success = pcall(1, 0);
			m_depth_list = nullptr;
			if (!success)
				throw std::runtime_error("Call to CalcForFrame failed");
		}

		bool Script::CalculateForSubtitle(int startFramenum, int lengthInFrames, int& depth) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref_CalcForSub);
			lua_pushinteger(L, startFramenum);
			lua_pushinteger(L, lengthInFrames);
			if (!pcall(2, 1))
				throw std::runtime_error("Call to CalcForSub failed");
			if (lua_isnil(L, -1))
				return false;
			int isnum;
			depth = (int)lua_tointegerx(L, -1, &isnum);
			if (!isnum)
				throw std::runtime_error("The value returned by CalcForSub is not a number");
			return true;
		}

		void Script::OnFinish() {
			if (ref_OnFinish == LUA_REFNIL)
				return;
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref_OnFinish);
			if (!pcall(0, 0))
				throw std::runtime_error("Call to OnFinish failed");
		}



		LPCSTR SetLuaFileParams = "[luafile]s";

		AVSValue SetLuaFile(AVSValue args, void* user_data, IScriptEnvironment* env) {
			Script::SetLuaFile(args[0].AsString());
			return AVSValue();
		}

	}
}
