project(Sub3D VERSION "1.0.2")
make_version_file(${PROJECT_VERSION} "3D subtitles renderer")

include_directories(
	${AVSTOOLS_INCLUDE_DIR}
)

set(HEADERS
	Filter.SRTRenderer.hpp
	Filter.XMLRenderer.hpp
	Tools.Depth.hpp
	Tools.Lua.hpp

	resource.h
)

set(SOURCES
	Filter.SRTRenderer.cpp
	Filter.XMLRenderer.cpp
	Tools.Depth.cpp
	Tools.Lua.cpp

	dllmain.cpp
	sub3d.rc
)

if (MINGW)
	list(APPEND SOURCES ${PROJECT_NAME}.mingw.def)
endif()

set_precompiled_header(stdafx.h stdafx.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} AvsTools Lua TinyXML2)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ${PROJECT_NAME})

install(FILES
	sub3d.avs
	sub3d-2.avs
	sub3d-3.avs
	tools3d.avsi
	donate-info.bat
	vcomp140.dll
	vsfilter.klite.dll
	VSFilter.mpc.dll
	sub3d.lua
	sub3d-old.lua
	sub3d.change.log
	DESTINATION bin COMPONENT ${PROJECT_NAME}
)
