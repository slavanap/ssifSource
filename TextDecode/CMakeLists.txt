project(TextDecode VERSION 0.1)

include_directories(
	${BASECLASSES_INCLUDE_DIR}
	${AVSTOOLS_INCLUDE_DIR}
)

set(HEADERS
	mainfilter.h
)

set(SOURCES
	dllmain.cpp
	mainfilter.cpp
)

set_precompiled_header(stdafx.h stdafx.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} AvsTools)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ${PROJECT_NAME})
