#pragma once

#include <string>

class Texture2D
{
public:
	Texture2D(const std::string& filePath,
		GLint wrap_s = GL_MIRRORED_REPEAT,
		GLint wrap_t = GL_MIRRORED_REPEAT,
		GLint min_filter = GL_LINEAR,
		GLint mag_filter = GL_LINEAR
	);

	~Texture2D();

	void setTexParami(GLenum option, GLint Param);
	bool getLoadSuccess() { return bLoadSuccess; }
	GLuint getTextureId() { return textureId; }

	int getWidth() { return width; }
	int getHeight() { return height; }
private:
	bool bLoadSuccess;
	GLuint textureId;
	int width;
	int height;
	int nrChannels;
};

