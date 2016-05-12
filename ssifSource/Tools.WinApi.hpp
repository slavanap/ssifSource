#pragma once

namespace Tools {

	namespace WinApi {

		extern HINSTANCE hInstance;

		void AddCurrentPathToPathEnv();
		void ErrorMessageA(DWORD dwError, LPSTR pBuffer, size_t nSize);
		void ConsolePrintErrorA(LPSTR desc, DWORD code);

		bool ConfidentRead(HANDLE hFile, LPVOID buffer, size_t cbSize);
		bool ConfidentWrite(HANDLE hFile, LPCVOID buffer, size_t cbSize);

		std::string IntToStr(int a);
		std::string format(LPCSTR fmt, int size, ...);
		std::string GetErrorMessage(DWORD code);
		std::string ExtractFilePath(const std::string& fn);
		bool IsDirectoryExists(LPCSTR szPath);

	}

}
