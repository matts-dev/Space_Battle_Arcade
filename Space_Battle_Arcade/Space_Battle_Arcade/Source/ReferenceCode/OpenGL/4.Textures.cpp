
#include"Utilities.h"
#include"Shader.h"
#include "Libraries/stb_image.h"


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

		GLuint EAO, VAO, VBO;
		Utils::generate4AttribRectangleElement(EAO, VAO, VBO);

		//Generate Texture in OpenGL
		GLuint texture1ID = 0;
		glGenTextures(1, &texture1ID);
		//glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth, tHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D); //mimaps basically gives us a table of scaled texture images 

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //just use nearest when scaling down
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //smooth out scaling upward 
		
		//free data once texture is loaded
		stbi_image_free(data);

		int tWidth2, tHeight2, nrChannels2;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* tex2Data = stbi_load("Textures/awesomeface3.png", &tWidth2, &tHeight2, &nrChannels2, 0);
		if (!tex2Data) { glfwTerminate(); return -1; }
		GLuint texture2ID = 0;
		glGenTextures(1, &texture2ID);
		//glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, texture2ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth2, tHeight2, 0, GL_RGB, GL_UNSIGNED_BYTE, tex2Data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(tex2Data);
		
		//prepare shader by setting which uniform corresponds to which GL_Texture
		shader3attribs.use();
		GLuint glText1Location = glGetUniformLocation(shader3attribs.getId(), "texture1");
		GLuint glText2Location = glGetUniformLocation(shader3attribs.getId(), "texture2");
		glUniform1i(glGetUniformLocation(shader3attribs.getId(), "texture1"), 0);
		glUniform1i(glGetUniformLocation(shader3attribs.getId(), "texture2"), 1);

		

		while (!glfwWindowShouldClose(window))
		{

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			//First Texture Code
			//shader3attribs.use();
			//glBindTexture(GL_TEXTURE_2D, texture1ID);
			//glBindVertexArray(VAO);
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			//Second Texture Code
			shader3attribs.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture1ID);
			
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, texture2ID);
			
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			//TEST
			//shader3attribs.use();
			//glActiveTexture(GL_TEXTURE1);
			//glBindTexture(GL_TEXTURE_2D, texture2ID);
			//glBindVertexArray(VAO);
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);
		}

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EAO);

		return 0;
	}


}

//int main() {
//	return Tex::main();
//}