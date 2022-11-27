include(FetchContent)

macro(LinkKLEIN TARGET ACCESS)
	FetchContent_Declare(
		klein
		GIT_REPOSITORY https://github.com/jeremyong/klein
		GIT_TAG v2.2.1
	)

	FetchContent_GetProperties(klein)

	if (NOT klein_POPULATED)
		FetchContent_Populate(klein)
	endif()

	# ACTUALLY - linking project will let us user their cmake setup. # only include the public headers, don't need full project. See README.md for the git repository
	#add_subdirectory(${klein_SOURCE_DIR} ${klein_BINARY_DIR} EXCLUDE_FROM_ALL)
	target_include_directories(${TARGET} ${ACCESS} ${klein_SOURCE_DIR}/public)
	target_compile_options( ${TARGET} ${ACCESS} -DWITH_KLEIN=1) #switch to enable/disable code around klein
	if(HTML_BUILD) 
		#message(STATUS "Building HTML Geometric Algebra")
		message(STATUS "HTML: KLEIN appending SIMD compile flag")
		#HTML_AppendLinkFlag(${EXECUTABLE_NAME} "-msimd128") #enable SIMD proposal so that we can compile klein (ERROR THIS ISN"T LINK FLAG")
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128") #enable SIMD proposal so that we can compile klein (limited browser support, so just do it for this project)
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -mssse3")
		target_compile_options( ${TARGET} ${ACCESS} -msimd128) 
		target_compile_options( ${TARGET} ${ACCESS} -msse -msse2 -msse3 -mssse3)
	elseif(MSVC)
		#windows doesn't need special SSE compile flags
	else()
		#set up SSE on linux
		target_compile_options( ${TARGET} ${ACCESS} -msse -msse2 -msse3 -mssse3)
	endif()

	#add_dependencies(${TARGET} klein)

endmacro()
