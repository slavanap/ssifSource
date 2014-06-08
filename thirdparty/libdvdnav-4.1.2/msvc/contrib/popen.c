#include <windows.h>
#include <process.h>
#include <io.h>
#define _POSIX_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


#include <windows.h>
//#include <msvcrt/io.h>
#include <io.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <internal/file.h>
//#define NDEBUG
//#include <msvcrtdbg.h>

#define PMAX 5
#define THREAD_SAFE __declspec(thread)

struct damn_windows {
	FILE *fd;
	int pid;
	HANDLE process;
};

THREAD_SAFE static struct damn_windows save_nodes[PMAX] = { 
	{ NULL,0,0}, 
	{ NULL,0,0}, 
	{ NULL,0,0}, 
	{ NULL,0,0}, 
};


BOOL WINAPI safe_popen_handler(DWORD CtrlType)
{
#ifdef DEBUG
	fprintf(stderr, "signal\r\n");
#endif
	return 1;
}


FILE *safe_popen (const char *cm, const char *md) /* program name, pipe mode */
{
  char *szCmdLine=NULL;
  char *szComSpec=NULL;
  char *s;
  int i;
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  BOOL result;
  STARTUPINFOA StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  if (cm == NULL)
    return NULL;

  szComSpec = getenv("COMSPEC");

  if (szComSpec == NULL)
  {
    szComSpec = strdup("cmd.exe");
    if (szComSpec == NULL)
      return NULL;
  } else {
	  szComSpec = strdup(szComSpec); // getenv is not for free()
  }

  s = max(strrchr(szComSpec, '\\'), strrchr(szComSpec, '/'));
  if (s == NULL)
    s = szComSpec;
  else
    s++;

  szCmdLine = malloc(strlen(s) + 4 + strlen(cm) + 1);
  if (szCmdLine == NULL)
  {
    free (szComSpec);
    return NULL;
  }

  strcpy(szCmdLine, s);
  s = strrchr(szCmdLine, '.');
  if (s)
    *s = 0;
  strcat(szCmdLine, " /C ");
  strcat(szCmdLine, cm);

  if ( !CreatePipe(&hReadPipe,&hWritePipe,&sa,1024))
  {
    free (szComSpec);
    free (szCmdLine);
    return NULL;
  }

  memset(&StartupInfo, 0, sizeof(STARTUPINFOA));
  StartupInfo.cb = sizeof(STARTUPINFOA);

  if (*md == 'r' ) {
	StartupInfo.hStdOutput = hWritePipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }
  else if ( *md == 'w' ) {
	StartupInfo.hStdInput = hReadPipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }

  result = CreateProcessA(szComSpec,
	                  szCmdLine,
			  NULL,
			  NULL,
			  TRUE,
			  CREATE_NEW_PROCESS_GROUP, // So we can send the signal
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  free (szComSpec);
  free (szCmdLine);

  if (result == FALSE)
  {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return NULL;
  }

  CloseHandle(ProcessInformation.hThread);


  if ( *md == 'r' )
      {
          int fd;
          fd = _open_osfhandle(hReadPipe,  O_RDONLY|O_BINARY);
          pf = _fdopen(fd, "rb");
          CloseHandle(hWritePipe);
      }
  else
      {
          pf = _fdopen( _open_osfhandle(hWritePipe, O_WRONLY|O_BINARY) , "wb");
          CloseHandle(hReadPipe);
      }
  
  for (i = 0;i < PMAX; i++)
      if (!save_nodes[i].fd) break;
  if (i>=PMAX)i=0;
  save_nodes[i].fd = pf;
  save_nodes[i].pid = ProcessInformation.dwProcessId;
  save_nodes[i].process = ProcessInformation.hProcess;
  
#ifdef DEBUG
  fprintf(stderr, "fd %p saved pid %d in pos %d\r\n", pf, save_nodes[i].pid, i);
#endif
  
  return pf;
}


int safe_pclose (FILE *pp)
{
	int i,ret=0;
    
#ifdef DEBUG
	fprintf(stderr, "pclose %p\r\n", pp);
#endif
	fclose(pp);
    
	for (i = 0;i < PMAX; i++) {
		if (save_nodes[i].fd == pp) {
            
            // Initially we attempted to call TerminateProcess() on either the hProcess, or the OpenProcess(pid)
            // but this failed, as it was actually killing "cmd" process, not unrar. And since unrar had not
            // exited yet, it would zombie cmd. 
            // So now we send a Console ctrl/break to the process-group, this includes cmd, and unrar.
			GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, save_nodes[i].pid);
#ifdef DEBUG
			fprintf(stderr, "pclose %p killing %d from pos %d %d\r\n", pp, save_nodes[i].pid,
                    i, ret);
#endif
			WaitForSingleObject(save_nodes[i].process, 1000);
			CloseHandle(save_nodes[i].process);
			save_nodes[i].process = 0;
			save_nodes[i].fd = NULL;
			save_nodes[i].pid = 0;
			break;
		}
	}
    
	return 0;
}

