#pragma once

#include "targetver.h"

// WinApi references
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

// ATL references
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlstr.h>

// AviSynth reference
#include <AviSynth\avisynth.h>

// DirectShow references
#include <streams.h>

// STD references
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <memory>

// useful defines
#define TEST(x, y) (((x) & (y)) != 0)
#define TESTALL(x, y) (((x) & (y)) == (y))
