project(CoreAVCWrapper)

set(HEADERS
)

set(SOURCES
	dllmain.cpp
	${PROJECT_NAME}.def
)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tools")
target_link_libraries(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT Tools)
