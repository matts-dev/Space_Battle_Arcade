
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"

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
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 idBasedColor;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					gl_PointSize = gl_Position.z;

					if(gl_VertexID % 2 == 0)
					{
						idBasedColor = vec3(1, 0, 0);
					} else {
						idBasedColor = vec3(0, 0, 1);
					}
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				in vec3 idBasedColor;
				out vec4 fragmentColor;

				void main(){
					fragmentColor = glm::vec4(idBasedColor, 1.f);
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
		camera.setPosition(0.0f, 0.0f, 3.0f);
		int width = 800;
		int height = 600;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);

		//LOAD BACKGROUND TEXTURE
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		float vertices[] = {
			//x      y      z   
			0.0f, 0.0f, 0.0f
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);

		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();


		std::vector<glm::vec3> points = {
			glm::vec3(1,1, 0.5),
			glm::vec3(-1, 1, -0.5),
			glm::vec3(0.5,-1, -1.5),
			glm::vec3(-0.5, 0.5, -5.5),
			glm::vec3(0.5,0.25, -10.5)
		};

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_PROGRAM_POINT_SIZE);

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			shader.use();

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view)); 

			//probably the worst way to render points in terms of efficiency, but allows each point to be defined by a model matrix. 
			//haven't learned about instancing yet, but perhaps it is applicable here.
			glBindVertexArray(vao);
			for (glm::vec3& position : points)
			{
				glm::mat4 model;
				model = glm::translate(model, position);
				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				glDrawArrays(GL_POINTS, 0, 1);
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