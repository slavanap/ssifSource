#pragma once

// WinApi references
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <strsafe.h>

// STD references
#include <algorithm>
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <climits>
#include <stdexcept>

template<typename T>
inline T sqr(T v) {
	return v * v;
}
