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

macro(configure_msvc_runtime link_type)
	if(MSVC)
		# Default to statically-linked runtime.
		if("${link_type}" STREQUAL "")
			set(link_type "static")
		endif()
		# Set compiler options.
		set(variables
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_MINSIZEREL
			CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_RELWITHDEBINFO
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_MINSIZEREL
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_RELWITHDEBINFO
		)
		if(${link_type} STREQUAL "static")
			message(STATUS "MSVC -> forcing use of statically-linked runtime.")
			foreach(variable ${variables})
				if(${variable} MATCHES "/MD")
					string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
				endif()
			endforeach()
		else()
			message(STATUS "MSVC -> forcing use of dynamically-linked runtime.")
			foreach(variable ${variables})
				if(${variable} MATCHES "/MT")
					string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
				endif()
			endforeach()
		endif()
	endif()
endmacro()
