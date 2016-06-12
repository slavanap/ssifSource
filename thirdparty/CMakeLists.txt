add_subdirectory(BaseClasses)
set(BASECLASSES_INCLUDE_DIR ${BASECLASSES_INCLUDE_DIR} PARENT_SCOPE)

add_subdirectory(tinyxml2)

project(Lua)
file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Lua/src/*.c Lua/src/*.h)
list(REMOVE_ITEM SOURCES Lua/src/luac.c)
add_library(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Third Party")
