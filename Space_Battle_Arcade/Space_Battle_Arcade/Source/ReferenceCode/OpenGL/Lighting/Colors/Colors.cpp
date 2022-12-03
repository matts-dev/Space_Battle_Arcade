
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

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				uniform vec3 objectColor;

				void main(){
					fragmentColor = vec4(lightColor * objectColor, 1.0f);
				}
			)";
	const char* lamp_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* lamp_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
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

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		float vertices[] = {
			//x      y      z   
			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f, 
			0.5f,  0.5f, -0.5f, 
			0.5f,  0.5f, -0.5f, 
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,

			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,

			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f, -0.5f, 
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 

			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f,  0.5f, 
			0.5f, -0.5f,  0.5f, 
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f,  0.5f, -0.5f,
			0.5f,  0.5f, -0.5f, 
			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
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

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.

		//GENERATE LAMP
		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		Shader shader(vertex_shader_src, frag_shader_src, false);
		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightPos(3.0, 1.0, -3.0);

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

			//draw object
			glm::mat4 model;
			model = glm::translate(model, objectPos);
			shader.use();
			shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform3f("objectColor", objectcolor.x, objectcolor.y, objectcolor.z);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			//draw light
			model = glm::mat4(1.f); //set model to identity matrix
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f));
			lampShader.use();
			lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			glBindVertexArray(lampVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);

		glDeleteVertexArrays(1, &lampVAO);
	}
}

//int main()
//{
//	true_main();
//}