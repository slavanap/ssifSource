//
// WinAPI miscellaneous tools 
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

#include <string>
#include <stdexcept>

namespace Tools {

	namespace WinApi {

		extern HINSTANCE hInstance;

#ifdef UNICODE
		typedef std::wstring tstring;
#else
		typedef std::string tstring;
#endif

		extern tstring szLibraryPath;

		void AddLibraryPathToPathEnv();
		BOOL InitLibrary(HMODULE hModule);
		void ErrorMessageA(DWORD dwError, LPSTR pBuffer, size_t nSize);
		void ConsolePrintErrorA(LPCSTR desc, DWORD code);

		bool ConfidentRead(HANDLE hFile, LPVOID buffer, size_t cbSize);
		bool ConfidentWrite(HANDLE hFile, LPCVOID buffer, size_t cbSize);

		std::string IntToStr(int a);
		std::string format(LPCSTR fmt, int size, ...);
		std::string GetErrorMessage(DWORD code);
		std::string ExtractFileDir(const std::string& fn);
		std::string ExtractFileName(const std::string& fn);
		bool IsDirectoryExists(LPCSTR szPath);

		std::string GetTempFilename();

		class UniqueIdHolder {
		public:
			UniqueIdHolder();
			~UniqueIdHolder();
			std::string GetPipePath() const;
			std::string MakePipeName(const std::string& name) const;
			int Id() { return _id; }
		private:
			HANDLE hSemaphore;
			int _id;
		};

		class ProcessHolder {
		public:
			static std::string BinPath;
			ProcessHolder(std::string exe_name, const std::string& exe_args, bool flag_debug);
			~ProcessHolder();
			void Resume();
		private:
			STARTUPINFOA SI;
			PROCESS_INFORMATION PI;
		};

		inline std::wstring towstring(const char* ch, int len, UINT cp) {
			if (len <= 0)
				return std::wstring();
			int ret = MultiByteToWideChar(cp, 0, ch, len, nullptr, 0);
			if (ret > 0) {
				std::wstring str((size_t)ret, 0);
				ret = MultiByteToWideChar(cp, 0, ch, len, &str[0], ret);
				if (ret > 0)
					return str;
			}
			throw std::invalid_argument("Can't convert MultiByteToWideChar");
		}

		inline std::string tostring(const wchar_t* wch, int len, UINT cp) {
			if (len <= 0)
				return std::string();
			BOOL cant_convert;
			int ret = WideCharToMultiByte(cp, 0, wch, len, nullptr, 0, nullptr, &cant_convert);
			if (ret > 0 && !cant_convert) {
				std::string str((size_t)ret, 0);
				ret = WideCharToMultiByte(cp, 0, wch, len, &str[0], ret, nullptr, &cant_convert);
				if (ret > 0 && !cant_convert)
					return str;
			}
			throw std::invalid_argument("Can't convert WideCharToMultiByte");
		}

		inline std::string tostring(const std::wstring& wstr, UINT cp) {
			return tostring(wstr.data(), (int)wstr.size(), cp);
		}

		inline std::wstring towstring(const std::string& str, UINT cp) {
			return towstring(str.data(), (int)str.size(), cp);
		}

#ifdef UNICODE
		inline std::string tostring(const tstring& tstr) { return tostring(tstr, CP_ACP); }
		inline std::wstring towstring(const tstring& tstr) { return tstr; }
#else
		inline std::string tostring(const tstring& tstr) { return tstr; }
		inline std::wstring towstring(const tstring& tstr) { return towstring(tstr, CP_ACP); }
#endif

		size_t string_pos_max(size_t pos1, size_t pos2);

	}

}
