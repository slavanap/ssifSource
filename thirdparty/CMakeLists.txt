if (MSVC)
	add_subdirectory(BaseClasses)
	set(BASECLASSES_INCLUDE_DIR ${BASECLASSES_INCLUDE_DIR} PARENT_SCOPE)
endif()

add_subdirectory(tinyxml2)

project(Lua)
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Lua/src/*.c Lua/src/*.h)
list(REMOVE_ITEM SOURCES Lua/src/luac.c)
add_library(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")

project(libufileread)
add_library(${PROJECT_NAME} STATIC ${PROJECT_NAME}.h ${PROJECT_NAME}.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")

project(libudfread)
add_library(${PROJECT_NAME} SHARED
	${PROJECT_NAME}.hpp
	${PROJECT_NAME}.cpp

	config.h
	udfread-version.h 

	libudfread/src/blockinput.h
	libudfread/src/default_blockinput.h
	libudfread/src/default_blockinput.c
	libudfread/src/ecma167.h
	libudfread/src/ecma167.c
	libudfread/src/udfread.h
	libudfread/src/udfread.c
	libudfread/src/udfread-version.c 
)
set_property(TARGET libudfread APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/libudfread/src")
set_property(TARGET libudfread APPEND PROPERTY COMPILE_DEFINITIONS LIBUDFREAD_EXPORTS HAVE_CONFIG_H)
target_link_libraries(${PROJECT_NAME} Shlwapi.lib)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")

add_subdirectory(MPC)
