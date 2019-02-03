cmake_minimum_required(VERSION 3.0)

project(Tests)

include_directories(
	${AVSTOOLS_INCLUDE_DIR}
)

set(HEADERS
	tests.hpp
)

set(SOURCES
	VideoCorrelation.Test.cpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} AvsTools Lua TinyXML2)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
