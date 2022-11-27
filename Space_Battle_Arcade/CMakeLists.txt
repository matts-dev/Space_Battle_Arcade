cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

message(STATUS "------------------------------------------------------------------------------------")

#-----------------------------------------------------------------------
# CMAKE REFACTOR STATUS AND TODOS
#
# phase 1) just use the headeronly libraries in the repo initialy, only replacing things like dynamic libraries
#	-currently working, but we are not compiling, must
# 
# phase 2) phase out the libraries being checked in with this repository, pull them down dynamically.
#
# phase 3) update dependencies
#
# phase 4) html compilation (may require refactoring not sure webgl2 supports our offscreen framebuffer rendering)
#-----------------------------------------------------------------------


set(EXECUTABLE_NAME SpaceBattleArcade)
project("${EXECUTABLE_NAME}")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS 
	Space_Battle_Arcade/Source/*.cpp 
	Space_Battle_Arcade/Source/*.h
	Space_Battle_Arcade/SourceExternal/*.cpp# TODO - once compiling see if perhaps can remove this and pull down our own dependencies
	Space_Battle_Arcade/SourceExternal/*.c# TODO - once compiling see if perhaps can remove this and pull down our own dependencies
	Space_Battle_Arcade/SourceExternal/*.h# TODO - once compiling see if perhaps can remove this and pull down our own dependencies
	)
add_executable(${EXECUTABLE_NAME} ${sources})
target_include_directories(${EXECUTABLE_NAME} PUBLIC 
	Space_Battle_Arcade/Source/
	Space_Battle_Arcade/SourceExternal/ # TODO - once compiling see if perhaps can remove this and pull down our own dependencies
)

# copy preload assets for to relative path for executable
#prepare copy assets to build directory
set(ASSET_COPY_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Space_Battle_Arcade/GameData/ CACHE BOOL "directory with the binary assets for game data" FORCE) 
set(ASSET_COPY_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/GameData/ CACHE BOOL "directory to put assets in relative to binary directory." FORCE) 
file(COPY ${ASSET_COPY_SOURCE} DESTINATION ${ASSET_COPY_DESTINATION})

# --- NOTE: html build is end goal, so copying this over from the gles2 project I have set up. ----
# if html build, instruct files to be preloaded, so that they can be loaded (eg textures)
#include(${SHARED_CMAKE_UTILITES_DIR}/UtilitiesHTML.cmake) #variable should be defined at toplevel cmakelists.txt
#HTMLPreloadDirectory(${EXECUTABLE_NAME} "${CMAKE_CURRENT_BINARY_DIR}/PreloadAssets/@PreloadAssets") #@ symbol renames path on left of @ to path on right of @; ie old/path@newpath
#HTMLUseTemplateHtmlFile(${EXECUTABLE_NAME} "${CMAKE_CURRENT_LIST_DIR}/html_output_template.html")

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

# --------------------------------------------------------------
# global variables
# --------------------------------------------------------------
if (CMAKE_EXECUTABLE_SUFFIX STREQUAL ".js")
	set(HTML_BUILD True CACHE BOOL "whether this is an html build" FORCE)
	add_compile_definitions(HTML_BUILD)
else()
	set(HTML_BUILD False CACHE BOOL "whether this is an html build" FORCE) 
endif()
message(STATUS "HTML_BUILD Value: ${HTML_BUILD}")

#set(SHARED_CMAKE_UTILITES_DIR "${CMAKE_CURRENT_LIST_DIR}/CMake" CACHE STRING "global location to include helpers from" FORCE) 
set(CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/CMake" CACHE STRING "global location to include helpers from" FORCE) 

# --------------------------------------------------------------
# Set up libraries
# --------------------------------------------------------------
# set up the engine library for all applications to link against
#add_subdirectory("Libraries/Engine")
#add_subdirectory("Libraries/TutorialEngine")
# link dependencies
if(NOT HTML_BUILD)
	message(STATUS "Engine Desktop Build")
	#only build this on desktop; emscripten does not need to build glfw manually, it ships with its own version
	include(${CMAKE_DIR}/LinkGLFW.cmake) 
	LinkGLFW(${EXECUTABLE_NAME} PUBLIC)

	#only build this on desktop; emscripten will use the headers included with emscripten expose opengl functions
	include(${CMAKE_DIR}/LinkGLAD.cmake) 
	LinkGLAD(${EXECUTABLE_NAME} PUBLIC)
	add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD) #haev imgui_impl_opengl3 pick up the corret loader.

	include(${CMAKE_DIR}/LinkOpenAL.cmake) 
	message(STATUS "________________________Link OpenAL________________________")
	LinkOpenAL(${EXECUTABLE_NAME} PUBLIC)
else()
	message(STATUS "Engine HTML Build: adding html linker options")
	target_link_options(${EXECUTABLE_NAME} PUBLIC -s USE_GLFW=3)	#link in emscripten glfw3; note that -lglfw is for the old glfw2
	target_link_options(${EXECUTABLE_NAME} PUBLIC -lopenal)		#link in emscripten OpenAL for audio.
endif()

# add in glm math functions to all builds
message(STATUS "________________________Link GLM________________________")
include(${CMAKE_DIR}/LinkGLM.cmake)
LinkGLM(${EXECUTABLE_NAME} PUBLIC)

#message(STATUS "________________________Link STB________________________")
#include(${CMAKE_DIR}/LinkSTB.cmake)
#LinkSTB(${EXECUTABLE_NAME} PUBLIC)

include(${CMAKE_DIR}/LinkAssimp.cmake) 
message(STATUS "________________________Link Assimp________________________")
LinkAssimp(${EXECUTABLE_NAME} PUBLIC)

# imgui is specific to engine project because of work arounds and is in a directory relative to engine library.
#include(${CMAKE_CURRENT_LIST_DIR}/CMake/EngineLinkImgui.cmake)
#EngineLinkIMGUI(${EXECUTABLE_NAME} PUBLIC)


add_compile_definitions(GLFW_INCLUDE_NONE) #stop GLFW and GLAD conflicts on linux; include order does not fix it on linux. Must be defined before include glfw; must not be defined when compiling glfw.


# change html build output so that it is runnable via server
if(HTML_BUILD) 
    message(STATUS "Building HTML")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    #"html_output_template.html"
endif()

message(STATUS " ------------------------------------------------------------------------------------")


