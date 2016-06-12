#include "stdafx.h"
#include "Tools.WinApi.hpp"

#include "common.h"
#include <ctime>

namespace Tools {

	namespace WinApi {

		HINSTANCE hInstance = nullptr;

		enum {
			PATH_BUFFER_LENGTH = 16 * 1024,
		};

		tstring szLibraryPath;

		void AddLibraryPathToPathEnv() {
			szLibraryPath.resize(PATH_BUFFER_LENGTH, 0);
			if (!GetModuleFileName(hInstance, &szLibraryPath[0], szLibraryPath.size()))
				goto error;
			size_t len;
			if (FAILED(StringCchLength(&szLibraryPath[0], szLibraryPath.size(), &len)))
				goto error;
			while (len > 0 && szLibraryPath[len - 1] != '\\') --len;
			if (len > 0) {
				szLibraryPath[len - 1] = ';';
				size_t path_len = GetEnvironmentVariable(TEXT("PATH"), &szLibraryPath[len], szLibraryPath.size() - len);
				if (path_len <= szLibraryPath.size() - len)
					SetEnvironmentVariable(TEXT("PATH"), &szLibraryPath[0]);
				szLibraryPath[len - 1] = '\\';
			}
			szLibraryPath.resize(len);
			return;
		error:
			throw std::exception("Can not retrieve program path");
		}

		BOOL InitLibrary(HMODULE hModule) {
			hInstance = hModule;
			try {
				AddLibraryPathToPathEnv();
				srand((unsigned int)time(nullptr));
				return TRUE;
			}
			catch (std::exception& e) {
				MessageBoxA(HWND_DESKTOP, e.what(), nullptr, MB_ICONERROR | MB_OK);
				return FALSE;
			}
		}

		void ErrorMessageA(DWORD dwError, LPSTR pBuffer, size_t nSize) {
			LPVOID pText = 0;
			::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pText, 0, nullptr);
			StringCchCopyA(pBuffer, nSize, (LPSTR)pText);
			LocalFree(pText);
		}

		void ConsolePrintErrorA(LPSTR desc, DWORD code) {
			if (desc == nullptr)
				desc = "ERROR";
			LPVOID lpMsgBuf;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)&lpMsgBuf, 0, nullptr);
			fprintf(stderr, "\n%s | 0x%.8x: %s\n", desc, code, (LPSTR)lpMsgBuf);
			LocalFree(lpMsgBuf);
		}

		bool ConfidentRead(HANDLE hFile, LPVOID buffer, size_t cbSize) {
			char *buf = reinterpret_cast<char*>(buffer);
			if (cbSize == 0) return true;
			if (cbSize < 0) return false;
			DWORD temp;
			do {
				if (!ReadFile(hFile, buf, cbSize, &temp, nullptr))
					return false;
				cbSize -= temp;
				buf += temp;
			} while (cbSize > 0);
			return true;
		}

		bool ConfidentWrite(HANDLE hFile, LPCVOID buffer, size_t cbSize) {
			const char *buf = reinterpret_cast<const char*>(buffer);
			if (cbSize == 0) return true;
			if (cbSize < 0) return false;
			DWORD temp;
			do {
				if (!WriteFile(hFile, buf, cbSize, &temp, nullptr))
					return false;
				cbSize -= temp;
				buf += temp;
			} while (cbSize > 0);
			return true;
		}

		std::string IntToStr(int a) {
			char buffer[32];
			_itoa_s(a, buffer, 32, 10);
			return buffer;
		}

		std::string format(LPCSTR fmt, int size, ...) {
			va_list al;
			va_start(al, size);
			std::string res;
			res.resize(size);
			vsprintf_s(const_cast<char*>(res.c_str()), size, fmt, al);
			res.resize(strlen(res.c_str()));
			va_end(al);
			return res;
		}

		std::string GetErrorMessage(DWORD code) {
			LPSTR lpMsgBuf;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)&lpMsgBuf, 0, nullptr);
			size_t len = strlen(lpMsgBuf);
			if (lpMsgBuf[len - 2] == 13 && lpMsgBuf[len - 1] == 10)
				lpMsgBuf[len - 2] = 0;
			std::string res = lpMsgBuf;
			LocalFree(lpMsgBuf);
			return res;
		}

		std::string ExtractFileDir(const std::string& fn) {
			size_t pos = fn.rfind("\\");
			if (pos == std::string::npos)
				return std::string();
			return fn.substr(0, pos + 1);
		}

		std::string ExtractFileName(const std::string& fn) {
			size_t pos = fn.rfind("\\");
			if (pos == std::string::npos)
				return fn;
			return fn.substr(pos + 1);
		}

		bool IsDirectoryExists(LPCSTR szPath) {
			DWORD dwAttrib = GetFileAttributesA(szPath);
			return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
				TEST(dwAttrib, FILE_ATTRIBUTE_DIRECTORY));
		}

		std::string GetTempFilename() {
			std::string path(MAX_PATH, 0);
			path.resize(GetTempPathA(path.length(), &path[0]));
			std::string filename(MAX_PATH, 0);
			GetTempFileNameA(path.c_str(), "sstmp", 0, &filename[0]);
			filename.resize(strlen(filename.c_str())); 
			if (filename.empty())
				throw std::runtime_error("Can't create temp file");
			return filename;
		}



		// class UniqueIdHolder

		LPCSTR globalName = "ssifSource";

		UniqueIdHolder::UniqueIdHolder() {
			_id = 0;
			while (true) {
				_id++;
				hSemaphore = CreateSemaphoreA(nullptr, 0, 1, format("Global\\%s_%d", 128, globalName, _id).c_str());
				if (GetLastError() == NOERROR)
					break;
				if (hSemaphore != nullptr)
					CloseHandle(hSemaphore);
			}
		}

		UniqueIdHolder::~UniqueIdHolder() {
			CloseHandle(hSemaphore);
		}

		std::string UniqueIdHolder::GetPipePath() const {
			return format("\\\\.\\pipe\\%s%04d", 128, globalName, _id);
		}

		std::string UniqueIdHolder::MakePipeName(const std::string& name) const {
			return format("%s\\%s", 128, GetPipePath().c_str(), name.c_str());
		}



		// class ProcessHolder

#if 0 //def _DEBUG
		std::string ProcessHolder::BinPath = "..\\..\\bin\\";
#else
		std::string ProcessHolder::BinPath;
#endif

		ProcessHolder::ProcessHolder(const std::string& exe_name, const std::string& exe_args, bool flag_debug) {
			memset(&SI, 0, sizeof(STARTUPINFO));
			SI.cb = sizeof(SI);
			SI.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
			SI.wShowWindow = flag_debug ? SW_SHOWNORMAL : SW_HIDE;

			memset(&PI, 0, sizeof(PROCESS_INFORMATION));
			PI.hProcess = INVALID_HANDLE_VALUE;

			std::string cmd = "\"" + exe_name + "\" " + exe_args;
			if (!CreateProcessA((BinPath + exe_name).c_str(), const_cast<LPSTR>(cmd.c_str()), nullptr, nullptr, false,
				CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
				nullptr, nullptr, &SI, &PI))
			{
				throw std::runtime_error("Error while launching " + exe_name);
			}
		}

		void ProcessHolder::Resume() {
			ResumeThread(PI.hThread);
		}

		ProcessHolder::~ProcessHolder() {
			if (PI.hProcess != INVALID_HANDLE_VALUE)
				TerminateProcess(PI.hProcess, 0);
		}



		size_t string_pos_max(size_t pos1, size_t pos2) {
			if (pos1 == std::string::npos)
				return pos2;
			if (pos2 == std::string::npos)
				return pos1;
			return std::max(pos1, pos2);
		}

	}

}
