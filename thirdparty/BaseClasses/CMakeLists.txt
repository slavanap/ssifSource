project(BaseClasses)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(HEADERS
	amextra.h
	amfilter.h
	cache.h
	checkbmi.h
	combase.h
	cprop.h
	ctlutil.h
	ddmm.h
	dllsetup.h
	dxmperf.h
	fourcc.h
	measure.h
	msgthrd.h
	mtype.h
	outputq.h
	perflog.h
	perfstruct.h
	pstream.h
	pullpin.h
	refclock.h
	reftime.h
	renbase.h
	schedule.h
	seekpt.h
	source.h
	streams.h
	strmctl.h
	sysclock.h
	transfrm.h
	transip.h
	videoctl.h
	vtrans.h
	winctrl.h
	winutil.h
	wxdebug.h
	wxlist.h
	wxutil.h
)

set(SOURCES
	amextra.cpp
	amfilter.cpp
	amvideo.cpp
	arithutil.cpp
	combase.cpp
	cprop.cpp
	ctlutil.cpp
	ddmm.cpp
	dllentry.cpp
	dllsetup.cpp
	mtype.cpp
	outputq.cpp
	perflog.cpp
	pstream.cpp
	pullpin.cpp
	refclock.cpp
	renbase.cpp
	schedule.cpp
	seekpt.cpp
	source.cpp
	strmctl.cpp
	sysclock.cpp
	transfrm.cpp
	transip.cpp
	videoctl.cpp
	vtrans.cpp
	winctrl.cpp
	winutil.cpp
	wxdebug.cpp
	wxlist.cpp
	wxutil.cpp
)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")
target_link_libraries(${PROJECT_NAME} Winmm.lib Strmiids.lib)

set(BASECLASSES_INCLUDE_DIR ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
