cmake_minimum_required(VERSION 3.0)
include(utils.cmake)

project(ssifSource VERSION "5.0.2")
set(PRODUCT_COMPANY "Vyacheslav Napadovsky")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Copyright (C) ${PRODUCT_COMPANY} 2012-2016")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Utility package for blu-ray files processing")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
install(FILES LICENSE DESTINATION bin)

include_directories( ${PROJECT_SOURCE_DIR} thirdparty )
link_directories( ${PROJECT_SOURCE_DIR}/thirdparty/avisynth )

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#configure_msvc_runtime("static")

add_definitions(-D _UNICODE)

add_subdirectory(thirdparty)

find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_subdirectory(CoreAVCWrapper)
add_subdirectory(FilmTester)
add_subdirectory(AvsTools)
add_subdirectory(LoadHelper)
add_subdirectory(ssifSource)

if (EXISTS "${PROJECT_SOURCE_DIR}/Sub3D/CMakeLists.txt")
	add_subdirectory(Sub3D)
endif()

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
SET(CPACK_NSIS_MODIFY_PATH ON)
INCLUDE(CPack)
