#pragma once

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <iostream>

//_WIN32 is defined in both x64 and x86
#ifdef _WIN32 

//Error Check OpenGL (MSCV version)
#ifdef _DEBUG
#define ec(func) UtilGL::clearErrorsGL(); /*clear previous errors */\
	func;			/*call function*/ \
	UtilGL::LogGLErrors(#func, __FILE__, __LINE__)
#else
#define ec(func) func
#endif //_DEBUG

#endif //_WIN32 

namespace UtilGL
{
	void clearErrorsGL();

	void LogGLErrors(const char* functionName, const char* file, int line);
}

