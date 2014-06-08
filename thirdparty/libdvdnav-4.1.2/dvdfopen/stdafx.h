#pragma once

#include <stdio.h>
#include <tchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <atlbase.h>
#include <atlstr.h>

extern "C" {
#include "dvd_types.h"
#include "dvd_reader.h"
#include "nav_types.h"
#include "ifo_types.h" /* For vm_cmd_t */
#include "dvdnav.h"
#include "dvdnav_events.h"
};

#include <string>

typedef std::basic_string<TCHAR> tstring;
