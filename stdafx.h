// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>

#include <assert.h>

// DirectShow references
#include <commdlg.h>
#include <streams.h>
#include <strsafe.h>

// Additional references

#include <limits.h>
#include <string>
#include <sstream>
#include <vector>
#include <tchar.h>
#include <iostream>
#include <ctime>

using namespace std;
