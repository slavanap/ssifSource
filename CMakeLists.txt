cmake_minimum_required(VERSION 2.8)
include(utils.cmake)

project(ssifSource)
include_directories( ${PROJECT_SOURCE_DIR} thirdparty )
link_directories( ${PROJECT_SOURCE_DIR}/thirdparty/avisynth )

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
elseif(MINGW)
	add_definitions(-D __MINGW_USE_VC2005_COMPAT)	# we need sizeof(time_t) == 8
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#configure_msvc_runtime("static")

add_definitions(-D _UNICODE)

add_subdirectory(thirdparty)

add_subdirectory(CoreAVCWrapper)
add_subdirectory(FilmTester)
add_subdirectory(AvsTools)
add_subdirectory(LoadHelper)
add_subdirectory(ssifSource)

if (EXISTS "${PROJECT_SOURCE_DIR}/Sub3D/CMakeLists.txt")
	add_subdirectory(Sub3D)
endif()

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME "ssifSource")
set(CPACK_PACKAGE_VENDOR "Vyacheslav Napadovsky")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BD3D processing tools")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ssifSource")
SET(CPACK_NSIS_MODIFY_PATH ON)
INCLUDE(CPack)
