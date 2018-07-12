project(AvsTools)

set(HEADERS
	common.h
	
	Filter.AdjustedColorDifference.hpp
	Filter.CropDetect.hpp
	Filter.HistogramMatching.hpp
	Filter.Pipe.hpp
	Filter.RestoreAlpha.hpp
	Filter.SequentialToSeekable.hpp
	Filter.SetLogger.hpp
	Filter.VideoCorrelation.hpp
	Tools.AviSynth.hpp
	Tools.AviSynth.Frame.hpp
	Tools.Motion.hpp
	Tools.Pipe.hpp
	Tools.WinApi.hpp
)

set(SOURCES
	Filter.AdjustedColorDifference.cpp
	Filter.CropDetect.cpp
	Filter.HistogramMatching.cpp
	Filter.Pipe.cpp
	Filter.RestoreAlpha.cpp
	Filter.SequentialToSeekable.cpp
	Filter.SetLogger.cpp
	Filter.VideoCorrelation.cpp
	Tools.AviSynth.cpp
	Tools.AviSynth.Frame.cpp
	Tools.Motion.cpp
	Tools.Pipe.cpp
	Tools.WinApi.cpp
)

set_precompiled_header(stdafx.h stdafx.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(AVSTOOLS_INCLUDE_DIR ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
