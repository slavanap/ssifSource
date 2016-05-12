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

	}

}
