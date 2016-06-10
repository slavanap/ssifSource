project(AvsTools)

set(HEADERS
	common.h
	
	Filter.CropDetect.hpp
	Filter.Pipe.hpp
	Filter.SequentialToSeekable.hpp
	Filter.SetLogger.hpp
	Tools.AviSynth.hpp
	Tools.Pipe.hpp
	Tools.WinApi.hpp
)

set(SOURCES
	Filter.CropDetect.cpp
	Filter.Pipe.cpp
	Filter.SequentialToSeekable.cpp
	Filter.SetLogger.cpp
	Tools.AviSynth.cpp
	Tools.Pipe.cpp
	Tools.WinApi.cpp
)

set_precompiled_header(stdafx.h stdafx.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(AVSTOOLS_INCLUDE_DIR ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
