project(ssifSource)

include_directories(${BASECLASSES_INCLUDE_DIR})

set(HEADERS
	mplsReader.h
	resource.h
	targetver.h
	DumpFilter.hpp
	Filter.mplsSource.hpp
	Filter.mplsSource2.hpp
	Filter.Pipe.hpp
	Filter.ssifSource.hpp
	Tools.AviSynth.hpp
	Tools.DirectShow.hpp
	Tools.Pipe.hpp
	Tools.WinApi.hpp
)

set(SOURCES
	dllmain.cpp
	DumpFilter.cpp
	Filter.mplsSource.cpp
	Filter.mplsSource2.cpp
	Filter.Pipe.cpp
	Filter.ssifSource.cpp
	Tools.AviSynth.cpp
	Tools.DirectShow.cpp
	Tools.Pipe.cpp
	Tools.WinApi.cpp
	ssifSource.rc
)

set_precompiled_header(stdafx.h stdafx.cpp)

list(APPEND SOURCES
	mplsReader.c
)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} BaseClasses)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ssifSource)

install(FILES
	sample.avs
	MpegSplitter_mod.ax
	libmfxsw64.dll
	vcomp90.dll
	vcomp90d.dll
	ldecod.exe
	MVCCombine.exe
	sample_decode.exe
	tsMuxeR.exe
	DESTINATION bin COMPONENT ssifSource
)

install(FILES
	x86/libmfxsw32.dll
	x86/vcomp90.dll
	x86/vcomp90d.dll
	x86/ldecod.exe
	x86/sample_decode.exe
	DESTINATION bin/x86 COMPONENT ssifSource
)
