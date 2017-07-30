#pragma once

#include"Utilities.h"
#include"Shader.h"
#include"libraries/stb_image.h"


namespace Tex {

	int main() {
		GLFWwindow* window = Utils::initOpenGl(800, 600);
		if (!window) return -1;

		Shader shader3attribs("4. VertShader_3attribs_pos_color_texcoord.glsl", "4. FragShader_3attribs_pos_color_texcoord.glsl");
		if (shader3attribs.createFailed()) { glfwTerminate(); return -1; }

		//LOAD TEXTURE DATA
		int tWidth, tHeight, nrChannels;
		unsigned char* data = stbi_load("Textures/container.jpg", &tWidth, &tHeight, &nrChannels, 0);
		if (!data) { glfwTerminate(); return-1; }

		//Generate Texture in OpenGL
		GLuint TextureID = 0;
		glGenTextures(1, &TextureID);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth, tHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D); //mimaps basically gives us a table of scaled texture images 

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //just use nearest when scaling down
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //smooth out scaling upward 

		GLuint EAO, VAO, VBO;
		Utils::generate4AttribRectangleElement(EAO, VAO, VBO);
		
		//free data once texture is loaded
		stbi_image_free(data);

		while (!glfwWindowShouldClose(window))
		{

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			shader3attribs.use();
			glBindTexture(GL_TEXTURE_2D, TextureID);
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);
		}


		return 0;
	}

}

int main() {
	return Tex::main();
}