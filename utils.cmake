macro(set_precompiled_header PrecompiledHeader PrecompiledSource)
	if (MSVC)
	    get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
	    set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
	    set_source_files_properties(${PrecompiledSource}
	    	PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_OUTPUTS "${PrecompiledBinary}")
		set_source_files_properties(${SOURCES}
			PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_DEPENDS "${PrecompiledBinary}")  
	endif()
	list(APPEND HEADERS ${PrecompiledHeader})
    list(APPEND SOURCES ${PrecompiledSource})
endmacro()
