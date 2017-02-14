project(MpegSplitter)

include_directories(
	../BaseClasses
	filters/parser/MpegSplitter
	include
	thirdparty/VirtualDub/h
)

set(HEADERS
)

set(SOURCES
	DSUtil/AudioParser.cpp
	DSUtil/DSMPropertyBag.cpp
	DSUtil/DSUtil.cpp
	DSUtil/FontInstaller.cpp
	DSUtil/GolombBuffer.cpp
	DSUtil/H264Nalu.cpp
	DSUtil/HdmvClipInfo.cpp
	DSUtil/vd.cpp
	DSUtil/vd_asm.cpp
	filters/FilterApp.cpp
	filters/parser/BaseSplitter/AsyncReader.cpp
	filters/parser/BaseSplitter/BaseSplitter.cpp
	filters/parser/BaseSplitter/BaseSplitterFile.cpp
	filters/parser/BaseSplitter/BaseSplitterFileEx.cpp
	filters/parser/BaseSplitter/MultiFiles.cpp
	filters/parser/MpegSplitter/MpegSplitter.cpp
	filters/parser/MpegSplitter/MpegSplitterFile.cpp
	filters/parser/MpegSplitter/MpegSplitterSettingsWnd.cpp
	filters/parser/MpegSplitter/stdafx.cpp
	thirdparty/VirtualDub/system/source/cpuaccel.cpp
	thirdparty/VirtualDub/system/source/memory.cpp
)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS NOSETTINGS STANDALONE_FILTER)
target_link_libraries(${PROJECT_NAME} BaseClasses libufileread Vfw32.lib Mincore.lib)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")
install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ${PROJECT_NAME})
