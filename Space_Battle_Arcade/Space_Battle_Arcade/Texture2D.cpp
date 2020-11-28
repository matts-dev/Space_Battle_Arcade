#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include "Texture2D.h"
#include "libraries\stb_image.h"



Texture2D::Texture2D(const std::string& filePath, GLint wrap_s, GLint wrap_t, GLint min_filter, GLint mag_filter) : bLoadSuccess(true), textureId(-1)
{
	unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) 
	{ 
		
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		stbi_image_free(data);
	}
	else
	{
		bLoadSuccess = false;
	}
}

Texture2D::~Texture2D()
{
	if (textureId >= 0)
	{
		glDeleteTextures(1, &textureId);
	}
}

/** Warning: this will clear what ever texture is currently bound*/
void Texture2D::setTexParami(GLenum option, GLint Param)
{
	if (bLoadSuccess && textureId > 0)
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, option, Param);
	}
}
