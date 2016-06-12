#pragma once

#include <string>

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
		void ConsolePrintErrorA(LPSTR desc, DWORD code);

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
			ProcessHolder(const std::string& exe_name, const std::string& exe_args, bool flag_debug);
			~ProcessHolder();
			void Resume();
		private:
			STARTUPINFOA SI;
			PROCESS_INFORMATION PI;
		};

		inline std::wstring towstring(const std::string& str, UINT cp) {
			int sz = MultiByteToWideChar(cp, 0, str.data(), (int)str.size(), NULL, 0);
			std::wstring ret(sz, 0);
			if (!MultiByteToWideChar(cp, 0, str.data(), (int)str.size(), &ret[0], (int)ret.size()))
				throw std::runtime_error("Can't convert std::string to std::wstring");
			return ret;
		}

		inline std::string tostring(const std::wstring& wstr, UINT cp) {
			int sz = WideCharToMultiByte(cp, WC_COMPOSITECHECK, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
			std::string ret(sz, 0);
			if (!WideCharToMultiByte(cp, 0, wstr.data(), (int)wstr.size(), &ret[0], (int)ret.size(), NULL, NULL))
				throw std::runtime_error("Can't convert std::wstring to std::string");
			return ret;
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
