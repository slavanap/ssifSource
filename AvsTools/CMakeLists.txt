project(AvsTools)

set(HEADERS
	common.h
	
	Filter.CropDetect.hpp
	Filter.Pipe.hpp
	Filter.RestoreAlpha.hpp
	Filter.SequentialToSeekable.hpp
	Filter.SetLogger.hpp
	Tools.AviSynth.hpp
	Tools.AviSynth.Frame.hpp
	Tools.HistogramMatching.hpp
	Tools.Motion.hpp
	Tools.Pipe.hpp
	Tools.WinApi.hpp
)

set(SOURCES
	Filter.CropDetect.cpp
	Filter.Pipe.cpp
	Filter.RestoreAlpha.cpp
	Filter.SequentialToSeekable.cpp
	Filter.SetLogger.cpp
	Tools.AviSynth.cpp
	Tools.AviSynth.Frame.cpp
	Tools.HistogramMatching.cpp
	Tools.Motion.cpp
	Tools.Pipe.cpp
	Tools.WinApi.cpp
)

if (EXISTS "${CMAKE_SOURCE_DIR}/proprietary/Tools.Motion.Proprietary.hpp")
	add_definitions(-D MOTION_PROPRIETARY)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/proprietary")
	list(APPEND HEADERS "${CMAKE_SOURCE_DIR}/proprietary/Tools.Motion.Proprietary.hpp")
endif()

set_precompiled_header(stdafx.h stdafx.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(AVSTOOLS_INCLUDE_DIR ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
