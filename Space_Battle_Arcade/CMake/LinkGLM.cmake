include(FetchContent)

macro(LinkGLM TARGET ACCESS)
	FetchContent_Declare(
		glm
		GIT_REPOSITORY https://github.com/DependencyMaster/glm.git
		GIT_TAG 0.9.8.0 #NOTE: this is an old version before api for identity matrix changed from mat() to mat(1.f)
	)

	FetchContent_GetProperties(glm)

	if (NOT glm_POPULATED)
		FetchContent_Populate(glm)
	endif()

	target_compile_definitions(${TARGET} PUBLIC GLM_FORCE_SILENT_WARNINGS=1) #silence glm warnings about nameless structs
	target_include_directories(${TARGET} ${ACCESS} ${glm_SOURCE_DIR})

	if(MSVC)
		set(GLM_NATVIS_LOC ${glm_SOURCE_DIR}/util/glm.natvis)
		message(STATUS "GLM with MSVC - recommened adding natvis. See this file for instructions: ${GLM_NATVIS_LOC}")#debug natvis
		#target_sources(${TARGET} INTERFACE  "${glm_SOURCE_DIR}/util/glm.natvis") manual adding does not seem to work. perhaps because not interface?
	endif()
endmacro()
