#include <windows.h>
#include <process.h>
#include <io.h>
#define _POSIX_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


#define POPEN_TYPE 2



#if (POPEN_TYPE==1)

//#include "safelib.h"
//#include "../argv.h"
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

//These are here because I don't believe in MSVC's prefixing affixation
#define dup _dup
#define dup2 _dup2
#define pipe _pipe
#define flushall _flushall
#define cwait _cwait


//Introducing a popen which doesn't return until it knows for sure of program launched or couldn't open -Nach

#define READ_FD 0
#define WRITE_FD 1

static struct fp_pid_link
{
  FILE *fp;
  int pid;
  struct fp_pid_link *next;
} fp_pids = { 0, 0, 0 };

/* This Windows popen/pclose implementation was taken straight from zsnes by Nach(?) */

static char *decode_string(char *str)
{
  size_t str_len = strlen(str), i = 0;
  char *dest = str;

  if ((str_len > 1) && ((*str == '\"') || (*str == '\'')) && (str[str_len-1] == *str))
  {
    memmove(str, str+1, str_len-2);
    str[str_len-2] = 0;
  }

  while (*str)
  {
    if (*str == '\\')
    {
      str++;
    }
    dest[i++] = *str++;
  }
  dest[i] = 0;
  return(dest);
}

static char *find_next_match(char *str, char match_char)
{
  char *pos = 0;

  while (*str)
  {
    if (*str == match_char)
    {
      pos = str;
      break;
    }
    if (*str == '\\')
    {
      if (str[1])
      {
        str++;
      }
      else
      {
        break;
      }
    }
    str++;
  }
  return(pos);
}

static char *get_param(char *str)
{
  static char *pos = 0;
  char *token = 0;

  if (str) //Start a new string?
  {
    pos = str;
  }

  if (pos)
  {
    //Skip delimiters
    while (*pos == ' ') { pos++; }
    if (*pos)
    {
      token = pos;

      //Skip non-delimiters
      while (*pos && (*pos != ' '))
      {
        //Skip quoted characters
        if ((*pos == '\"') || (*pos == '\''))
        {
          char *match_pos = 0;
          if ((match_pos = find_next_match(pos+1, *pos)))
          {
            pos = match_pos;
          }
        }
        //Skip escaped spaces
        if (*pos == '\\') { pos++; }
        pos++;
      }
      if (*pos) { *pos++ = '\0'; }
    }
  }
  return(token);
}

static size_t count_param(char *str)
{
  size_t i = 0;

  while (*str)
  {
    //Skip delimiters
    while (*str == ' ') { str++; }
    //Skip non-delimiters
    while (*str && (*str != ' '))
    {
      //Skip quoted characters
      if ((*str == '\"') || (*str == '\''))
      {
        char *match_str = 0;
        if ((match_str = find_next_match(str+1, *str)))
        {
          str = match_str;
        }
      }
      //Skip escaped spaces
      if (*str == '\\') { str++; }
      str++;
    }
    i++;
  }
  return(i);
}

static char **build_argv(char *str)
{
  size_t argc = count_param(str);
  char **argv = (char **)malloc(sizeof(char *)*(argc+1));

  if (argv)
  {
    char *p, **argp = argv;
    for (p = get_param(str); p; p = get_param(0), argp++)
    {
      *argp = decode_string(p);
    }
    *argp = 0;
    return(argv);
  }
  return(0);
}




FILE *safe_popen(char *command, const char *mode)
{
  FILE *ret = 0;
  char **argv = build_argv(command);
  if (argv)
  {
    int filedes[2];

    if (mode && (*mode == 'r' || *mode == 'w') &&
        !pipe(filedes, 512, (mode[1] == 'b' ? O_BINARY : O_TEXT) | O_NOINHERIT))
    {
      int fd_original;
      FILE *fp;

      if (*mode == 'r')
      {
        fd_original = dup(STDOUT_FILENO);
        dup2(filedes[WRITE_FD], STDOUT_FILENO);
        close(filedes[WRITE_FD]);
        if (!(fp = fdopen(filedes[READ_FD], mode)))
        {
          close(filedes[READ_FD]);
        }
      }
      else
      {
        fd_original = dup(STDIN_FILENO);
        dup2(filedes[READ_FD], STDIN_FILENO);
        close(filedes[READ_FD]);
        if (!(fp = fdopen(filedes[WRITE_FD], mode)))
        {
          close(filedes[WRITE_FD]);
        }
      }

      if (fp)
      {
        intptr_t childpid;
        flushall();

        childpid = spawnvp(P_NOWAIT, argv[0], (const char* const*)argv);
        if (childpid > 0)
        {
          struct fp_pid_link *link = &fp_pids;
          while (link->next)
          {
            link = link->next;
          }

          link->next = (struct fp_pid_link *)malloc(sizeof(struct fp_pid_link));
          if (link->next)
          {
            link->next->fp = fp;
            link->next->pid = childpid;
            link->next->next = 0;
            ret = fp;
          }
          else
          {
            fclose(fp);
            TerminateProcess((HANDLE)childpid, 0);
            cwait(0, childpid, WAIT_CHILD);
          }
        }
        else
        {
          fclose(fp);
        }
      }

      if (*mode == 'r')
      {
        dup2(fd_original, STDOUT_FILENO);
      }
      else
      {
        dup2(fd_original, STDIN_FILENO);
      }
      close(fd_original);
    }
    free(argv);
  }
  return(ret);
}

int safe_pclose(FILE *fp)
{
  struct fp_pid_link *link = &fp_pids;

  while (link->next && link->next->fp != fp)
  {
    link = link->next;
  }
  if (link->next->fp == fp)
  {
    struct fp_pid_link *dellink = link->next;


//	if (!feof(fp)) TerminateProcess(link->next->pid, 999);
    fclose(fp);
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, link->next->pid);
    cwait(0, link->next->pid, WAIT_CHILD);
    link->next = link->next->next;
    free(dellink);
  }
  return 0;
}

#endif // POPEN_TYPE 1




#if (POPEN_TYPE==2)


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

  //SetConsoleCtrlHandler(safe_popen_handler, TRUE);

#if 1
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
#else

  szComSpec = strdup("unrar.exe");
  szCmdLine = strdup(cm);

#endif

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

  //pf->_tmpfname = ProcessInformation.hProcess;
	for (i = 0;i < PMAX; i++)
		if (!save_nodes[i].fd) break;
	if (i>=PMAX)i=0;
	save_nodes[i].fd = pf;
	save_nodes[i].pid = ProcessInformation.dwProcessId;
	save_nodes[i].process = ProcessInformation.hProcess;
	//CloseHandle(ProcessInformation.hProcess);

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



#endif
