#include "OpenGLHelpers.h"



void UtilGL::clearErrorsGL()
{
	while (glGetError() != GL_NO_ERROR);
}

void UtilGL::LogGLErrors(const char* functionName, const char* file, int line)
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
