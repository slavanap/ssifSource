#include "stdafx.h"
#include "Tools.WinApi.hpp"

namespace Tools {

	namespace WinApi {

		HINSTANCE hInstance = nullptr;

		enum {
			PATH_BUFFER_LENGTH = 8192,
		};

		void AddCurrentPathToPathEnv() {
			std::wstring exe_filename(PATH_BUFFER_LENGTH, 0);
			if (!GetModuleFileNameW(hInstance, &exe_filename[0], PATH_BUFFER_LENGTH))
				throw std::exception("Can not retrieve program path");
			size_t len;
			StringCchLengthW(exe_filename.c_str(), PATH_BUFFER_LENGTH, &len);
			while (len > 0 && exe_filename[len - 1] != '\\') --len;
			if (len > 0) {
				exe_filename[len - 1] = ';';
				size_t path_len = GetEnvironmentVariableW(L"PATH", &exe_filename[len], PATH_BUFFER_LENGTH - len);
				if (path_len <= PATH_BUFFER_LENGTH - len) {
					SetEnvironmentVariableW(L"PATH", &exe_filename[0]);
				}
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
			if (desc == NULL)
				desc = "ERROR";
			LPVOID lpMsgBuf;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)&lpMsgBuf, 0, NULL);
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
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)&lpMsgBuf, 0, NULL);
			size_t len = strlen(lpMsgBuf);
			if (lpMsgBuf[len - 2] == 13 && lpMsgBuf[len - 1] == 10)
				lpMsgBuf[len - 2] = 0;
			std::string res = lpMsgBuf;
			LocalFree(lpMsgBuf);
			return res;
		}

		std::string ExtractFilePath(const std::string& fn) {
			size_t pos = fn.rfind("\\");
			if (pos == std::string::npos)
				return std::string();
			return std::string(fn.c_str(), pos);
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
			filename.resize(GetTempFileNameA(path.c_str(), "sstmp", 0, &filename[0]));
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

		ProcessHolder::ProcessHolder(const std::string& exe_name, const std::string& exe_args, bool flag_debug) {
			memset(&SI, 0, sizeof(STARTUPINFO));
			SI.cb = sizeof(SI);
			SI.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
			SI.wShowWindow = flag_debug ? SW_HIDE : SW_SHOWNORMAL;

			memset(&PI, 0, sizeof(PROCESS_INFORMATION));
			PI.hProcess = INVALID_HANDLE_VALUE;

			if (!CreateProcessA(exe_name.c_str(), const_cast<LPSTR>(exe_args.c_str()), nullptr, nullptr, false,
				CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
				nullptr, nullptr, &SI, &PI))
			{
				throw std::runtime_error("Error while launching %s" + exe_name);
			}
		}

		void ProcessHolder::Resume() {
			ResumeThread(PI.hThread);
		}

		ProcessHolder::~ProcessHolder() {
			if (PI.hProcess != INVALID_HANDLE_VALUE)
				TerminateProcess(PI.hProcess, 0);
		}

	}

}
