project(ssifSource VERSION ${ssifSource_VERSION})
make_version_file(${PROJECT_VERSION} "AviSynth plugin for ssif & mpls files decoding")

include_directories(
	${BASECLASSES_INCLUDE_DIR}
	${AVSTOOLS_INCLUDE_DIR}
)

set(HEADERS
	mplsReader.h
	resource.h
	DumpFilter.hpp
	Filter.mplsSource.hpp
	Filter.mplsSource2.hpp
	Filter.ssifSource.hpp
	Tools.DirectShow.hpp
)

set(SOURCES
	dllmain.cpp
	DumpFilter.cpp
	Filter.mplsSource.cpp
	Filter.mplsSource2.cpp
	Filter.ssifSource.cpp
	Tools.DirectShow.cpp
)

set_precompiled_header(stdafx.h stdafx.cpp)

list(APPEND SOURCES
	ssifSource.rc
	mplsReader.c
)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} BaseClasses AvsTools libufileread)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ${PROJECT_NAME})

install(FILES
	ssifSource.avs
	libmfxsw64.dll
	vcomp90.dll
	vcomp90d.dll
	ldecod.exe
	MVCCombine.exe
	sample_decode.exe
	tsMuxeR.exe
	DESTINATION bin COMPONENT ${PROJECT_NAME}
)

install(FILES
	x86/libmfxsw32.dll
	x86/vcomp90.dll            # for ldecod.exe only
	x86/vcomp90d.dll           # for ldecod.exe only
	x86/ldecod.exe
	x86/sample_decode.exe
	DESTINATION bin/x86 COMPONENT ${PROJECT_NAME}
)
