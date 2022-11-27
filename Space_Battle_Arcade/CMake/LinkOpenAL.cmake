#inspired from https://github.com/Hoshiningen/OpenGL-Template

# THIS FILE NEEDS TESTING

include(FetchContent)

macro(LinkOpenAL TARGET ACCESS)
	FetchContent_Declare(
		openal_soft
		GIT_REPOSITORY https://github.com/kcat/openal-soft.git
		GIT_TAG 1.21.1
	)

	FetchContent_GetProperties(openal_soft)

	if (NOT openal_soft_POPULATED)
		FetchContent_Populate(openal_soft)

		# debug
		message(STATUS "openal_soft source dir: ${openal_soft_SOURCE_DIR}")
		message(STATUS "openal_soft populated: ${openal_soft_POPULATED}")

		set(ALSOFT_INSTALL					OFF CACHE BOOL "don't install" FORCE)
		set(ALSOFT_INSTALL_CONFIG			OFF CACHE BOOL "don't install features" FORCE)
		set(ALSOFT_INSTALL_HRTF_DATA		OFF CACHE BOOL "don't install features" FORCE)
		set(ALSOFT_INSTALL_AMBDEC_PRESETS	OFF CACHE BOOL "don't install features" FORCE)
		set(ALSOFT_INSTALL_EXAMPLES			OFF CACHE BOOL "don't install features" FORCE)
		set(ALSOFT_INSTALL_UTILS			OFF CACHE BOOL "don't install features" FORCE)
		
		set(ALSOFT_EXAMPLES					OFF CACHE BOOL "don't build examples" FORCE)
		set(ALSOFT_UTILS					OFF CACHE BOOL "don't build the utils" FORCE)

		# This excludes openal from being rebuilt when ALL_BUILD is built
		# it will only be built when a target is built that has a dependency on openal
		add_subdirectory(${openal_soft_SOURCE_DIR} ${openal_soft_BINARY_DIR} EXCLUDE_FROM_ALL)

		# Set the target's folders
		set_target_properties(OpenAL PROPERTIES FOLDER ${PROJECT_NAME}/thirdparty)
	endif()

	target_include_directories(${TARGET} ${ACCESS} ${openal_soft_SOURCE_DIR}/include)
	target_link_libraries(${TARGET} ${ACCESS} OpenAL)

	add_dependencies(${TARGET} OpenAL)
endmacro()