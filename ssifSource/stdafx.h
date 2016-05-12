#pragma once

//#define _CRT_SECURE_NO_WARNINGS
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlstr.h>

#include <AviSynth\avisynth.h>

//#include <assert.h>

// DirectShow references
//#include <commdlg.h>
#include <streams.h>

// Additional references
//#include <limits.h>
//#include <tchar.h>
//#include <ctime>

//#pragma warning(push)
//#pragma warning(disable:4995)
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
//#pragma warning(pop)

#define TEST(x, y) (((x) & (y)) != 0)
#define TESTALL(x, y) (((x) & (y)) == (y))
