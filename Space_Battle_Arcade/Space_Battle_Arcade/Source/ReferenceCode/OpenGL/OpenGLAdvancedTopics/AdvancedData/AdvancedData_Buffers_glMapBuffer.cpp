#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/InputTracker.h"
#include <gtx/wrap.hpp>

namespace
{
	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec3 inColor;				
				
				out vec3 color;

				void main(){
					gl_Position = vec4(position, 1);
					color = inColor;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec3 color;

				void main(){
					fragmentColor = vec4(color, 1.0f);
				}
			)";

	int vertexIndex = 0;
	enum Modify : int {X = 0, Y, Z, RED, GREEN, BLUE}; //namespaced so safe
	Modify activeValue = Modify::RED;
	bool shouldReadColorValueFromOpenGL = true;
	bool colorDirty = true;
	float targetValue = 0.0f;

	float clamp(float val, float min, float max)
	{
		val = val < min ? min : val;
		val = val > max ? max : val;
		return val;
	}

	void processInput(GLFWwindow* window)
	{
		static InputTracker input;
		input.updateState(window);

		for (int key = GLFW_KEY_1; key <=GLFW_KEY_6; ++key)
		{
			if (input.isKeyJustPressed(window, key))
			{
				vertexIndex = key - GLFW_KEY_1;
				shouldReadColorValueFromOpenGL = true;
			}
		}
		for (int key = GLFW_KEY_X; key <= GLFW_KEY_Z; ++key)
		{
			if (input.isKeyJustPressed(window, key))
			{
				activeValue = static_cast<Modify>(key - GLFW_KEY_X);
				shouldReadColorValueFromOpenGL = true;
			}
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_R))
		{
			activeValue = Modify::RED;
			shouldReadColorValueFromOpenGL = true;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_G))
		{
			activeValue = Modify::GREEN;
			shouldReadColorValueFromOpenGL = true;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_B))
		{
			activeValue = Modify::BLUE;
			shouldReadColorValueFromOpenGL = true;
		}
		if (!shouldReadColorValueFromOpenGL && !colorDirty)
		{
			if (input.isKeyJustPressed(window, GLFW_KEY_EQUAL))
			{
				targetValue += 0.1f;
				targetValue = clamp(targetValue, activeValue < Modify::RED ? -1.f : 0.f, 1.f);
				colorDirty = true;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_MINUS))
			{
				targetValue -= 0.1f;
				targetValue = clamp(targetValue, activeValue < Modify::RED ? -1.f : 0.f, 1.f);
				colorDirty = true;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	void true_main()
	{
		GLFWwindow* window = init_window(800, 600);

		glViewport(0, 0, 800, 600);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		float tri1[] = {
			0.5f, 0.5f, 0.0f,	  0.0f, 0.0f, 1.0f,
			0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
			-0.5f, 0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
		};

		float tri2[] = {
			0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tri1) + sizeof(tri2), nullptr, GL_DYNAMIC_DRAW); //null just allocates the memory without actually filling anything into them memory
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)) );
		glEnableVertexAttribArray(1);

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.
		
		//-------------------------------------------------- advanced data ---------------------------------------------------------------------

		//demo buffering two separate sets of vertice data
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri1), tri1);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(tri1), sizeof(tri2), tri2);

		//-----------------------------------------------------------------------------------------------------------------------

		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &vertex_shader_src, nullptr);
		glCompileShader(vertShader);
		verifyShaderCompiled("vertex shader", vertShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &frag_shader_src, nullptr);
		glCompileShader(fragShader);
		verifyShaderCompiled("fragment shader", fragShader);

		GLuint shaderProg = glCreateProgram();
		glAttachShader(shaderProg, vertShader);
		glAttachShader(shaderProg, fragShader);
		glLinkProgram(shaderProg);
		verifyShaderLink(shaderProg);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);


		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe mode
		while (!glfwWindowShouldClose(window))
		{
			processInput(window);
			//-------------------------------------------------- advanced data ---------------------------------------------------------------------
			size_t vertsize = sizeof(tri1) / 3;
			size_t offset = (vertexIndex * vertsize) + (activeValue * sizeof(float)); 			// (skip to vertex) + (currentComponent)

			if (shouldReadColorValueFromOpenGL)
			{
				char* ptr = reinterpret_cast<char*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
				assert(ptr != nullptr);
				ptr += offset;

				targetValue = *reinterpret_cast<float*>(ptr);
				glUnmapBuffer(GL_ARRAY_BUFFER);

				shouldReadColorValueFromOpenGL = false;
			}
			if (colorDirty)
			{
				char* ptr = reinterpret_cast<char*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
				assert(ptr != nullptr);
				ptr += offset;

				float* outLoc = reinterpret_cast<float*>(ptr);
				*outLoc = targetValue;
				std::cout << "writing: " << targetValue << std::endl;
				glUnmapBuffer(GL_ARRAY_BUFFER);

				colorDirty = false;
			}
			// -------------------------------------------------------------------------------------------------------------------------------------

			glClearColor(0,0,0, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(shaderProg);
			glBindVertexArray(vao);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}
}

//int main()
//{
//	true_main();
//}