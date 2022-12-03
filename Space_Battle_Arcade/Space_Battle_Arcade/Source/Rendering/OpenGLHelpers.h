#pragma once

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <iostream>

#include "Game/OptionalCompilationMacros.h"


//Error Check OpenGL (MSCV version)
#if _DEBUG | ERROR_CHECK_GL_RELEASE 
#define ec(func) UtilGL::clearErrorsGL(); /*clear previous errors */\
	func;			/*call function*/ \
	UtilGL::LogGLErrors(#func, __FILE__, __LINE__)
#else
#define ec(func) func
#endif //_DEBUG

//_WIN32 is defined in both x64 and x86
#ifdef _WIN32 
#endif //_WIN32 

namespace UtilGL
{

	inline void clearErrorsGL()
	{
		unsigned long error = GL_NO_ERROR;
		do
		{
			//if you get stuck in an infinite loop here on shutdown, make sure you're calling super postConstruct with GPU resources; that is what hooks into events to release GPU resources.
			//inifinte loop here means you probably no longer have an opengl context.
			error = glGetError();
			//if (error != GL_NO_ERROR) { std::cerr << "Cleared error:" << std::hex << error << std::endl;}
		} while (error != GL_NO_ERROR);
	}

	inline void LogGLErrors(const char* functionName, const char* file, int line)
	{
		bool bError = false;
		//GL_NO_ERROR is defined as 0; which means we can use it in condition statements like below
		while (GLenum error = glGetError())
		{
			std::cerr << "OpenGLError:" << std::hex << error << " " << functionName << " " << file << " " << line << std::endl;
#ifdef _WIN32 
			__debugbreak();
#endif //_WIN32
		}
	}

}

