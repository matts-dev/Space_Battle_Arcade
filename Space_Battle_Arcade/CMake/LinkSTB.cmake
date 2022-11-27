
include(FetchContent)

#stb is a library used to read files, like image files.
macro(LinkSTB TARGET ACCESS)
	FetchContent_Declare(
		stb
		GIT_REPOSITORY https://github.com/DependencyMaster/stb
		GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a
	)

	FetchContent_GetProperties(stb)

	if (NOT stb_POPULATED)
		FetchContent_Populate(stb)
	endif()

	target_include_directories(${TARGET} ${ACCESS} ${stb_SOURCE_DIR})
endmacro()
