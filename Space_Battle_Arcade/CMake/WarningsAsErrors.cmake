#############################################
# Treat warnings as errors
#############################################

macro(WarningsAsErrors TARGET)
if ( CMAKE_COMPILER_IS_GNUCC )
	target_compile_options("${TARGET}" PRIVATE -Wall -Wextra) #compiler warnings as errors
	# clang/gcc use -Wunused-parameter.

	#below do not seem to work with gcc
	#target_compile_options("${TARGET}" PRIVATE --fatal-warnings) #linker warnings as errors (doesn't seem to work)
	#target_compile_options("${TARGET}" PRIVATE -Wl) #linker warnings as errors (doesn't seem to work)
endif()
if ( MSVC )
	target_compile_options("${TARGET}" PRIVATE /W4) #treat compiler warnigns as errors
	target_compile_options("${TARGET}" PRIVATE /WX) #treat linker warnings as errors

	#target_compile_options("${TARGET}" PRIVATE /wd4100) #ignore warning about unused formal parameters in virtual base functions.

	#potential fix for \windows-default\cl : Command line warning D9025: overriding '/W3' with '/W4'
	# if not wanting to convert to cmake version 3.15 or higher.
	#https://stackoverflow.com/questions/58708772/cmake-project-in-visual-studio-gives-flag-override-warnings-command-line-warnin
	string(REGEX REPLACE "/W[3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") #fix Command line warning D9025: overriding '/W3' with '/W4'
endif()
endmacro()