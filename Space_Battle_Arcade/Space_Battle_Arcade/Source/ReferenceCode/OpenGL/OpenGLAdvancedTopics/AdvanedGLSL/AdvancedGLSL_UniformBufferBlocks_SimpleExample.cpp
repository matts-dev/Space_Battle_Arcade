#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"
#include <cassert>

namespace
{
	CameraFPS camera(45.0f, -90.f, 0.f);

	float lastFrameTime = 0.f;
	float deltaTime = 0.0f;

	float pitch = 0.f;
	float yaw = -90.f;

	float FOV = 45.0f;

	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 inTexCoord;
				
				out vec2 texCoord;
				layout (std140) uniform Matrices
				{
					uniform mat4 view;
					uniform mat4 projection;
				};
				uniform mat4 model;

				void main(){
					texCoord = inTexCoord; //reminder, since this is an out variable it will be subject to fragment interpolation (which is what we want)
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* red_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 texCoord;
				vec4 color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
				void main(){
					fragmentColor = color;
				}
			)";
	const char* green_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 texCoord;
				vec4 color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
				void main(){
					fragmentColor = color;
				}
			)";
	const char* blue_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 texCoord;
				vec4 color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
				void main(){
					fragmentColor = color;
				}
			)";
	const char* yellow_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 texCoord;
				vec4 color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
				void main(){
					fragmentColor = color;
				}
			)";
	void processInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		camera.handleInput(window, deltaTime);
	}

	void true_main()
	{
		camera.setPosition(0.0f, 0.0f, 8.0f);
		int width = 800;
		int height = 600;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


		float vertices[] = {
			//x      y      z       s     t
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		GLuint ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);

		GLuint UBOMatrixBindingSlot = 0;
		glBindBufferBase(GL_UNIFORM_BUFFER, UBOMatrixBindingSlot, ubo);
		
		Shader redshader(vertex_shader_src, red_frag_shader_src, false);
		Shader greenshader(vertex_shader_src, green_frag_shader_src, false);
		Shader blueshader(vertex_shader_src, blue_frag_shader_src, false);
		Shader yellowshader(vertex_shader_src, yellow_frag_shader_src, false);

		std::vector<Shader*> shaders = { &redshader, &greenshader, &blueshader, &yellowshader };

		for (Shader* shader : shaders)
		{
			shader->use();
			GLuint shaderid = shader->getId();
			GLuint uniformLocIdx = glGetUniformBlockIndex(shaderid, "Matrices");
			glUniformBlockBinding(shaderid, uniformLocIdx, UBOMatrixBindingSlot);
		}

		glEnable(GL_DEPTH_TEST);

		glm::vec3 cubePositions[] = {
			glm::vec3( 2.0f,  2.0,  0.0f),
			glm::vec3(-2.0f,  2.0f,  0.0f),
			glm::vec3( 2.0f, -2.0f, -0.0f),
			glm::vec3(-2.0f, -2.0f, -0.0f)
		};
		assert( sizeof(cubePositions)/sizeof(glm::vec3) == shaders.size() );

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);
			//-------------------------------------------------------------------------------------------------------------------------
			//update uniform buffer for all shaders!
			glBindBuffer(GL_UNIFORM_BUFFER, ubo);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
			//-------------------------------------------------------------------------------------------------------------------------

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				shaders[i]->use();

				glm::mat4 model;
				float angle = 20.0f * i;
				model = glm::translate(model, cubePositions[i]);
				shaders[i]->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));

				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

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