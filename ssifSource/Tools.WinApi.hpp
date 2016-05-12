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

	}

}
