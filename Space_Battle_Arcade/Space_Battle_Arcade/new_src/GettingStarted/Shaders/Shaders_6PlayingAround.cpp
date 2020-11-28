#pragma once
#include<iostream>
#include<iomanip>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../../nu_utils.h"
#include "../../../Shader.h"


namespace
{
	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec3 color1;	
				layout (location = 2) in vec3 color2;	
				layout (location = 3) in vec3 color3;	
				
				out vec4 vertColor;

				uniform float time;
				uniform float timeoffset;

				void main(){
					float sinewave1 = sin(time);
					vec3 timeoffsets = vec3(time, time + timeoffset, time + 2 * timeoffset);
					vec3 waves = sin(timeoffsets);	    //apply sin to all times, [-1, 1]

					//don't need below anymore, but good to know that you can do these operations on vectors in glsl
					//waves /= 2.0f;					//make waves from range [-0.5, 0.5]
					//waves += 0.5f;					//shift waves to [0, 1]

					//simply add up the colors and use waves to determine whether color is active or not
					//since non-used colors will be 0 this doesn't ever subtract out colors (because 0 * -sinwave is still 0)
					//because opengl interprets negative color values as black, multiplying by a negative wave gives us the effect we want (but we need a black background)
					vertColor = vec4(color1 * waves.x, 1.0f);
					vertColor += vec4(color2 * waves.y, 1.0f);
					vertColor += vec4(color3 *waves.z, 1.0f);					

					gl_Position = vec4(position, 1);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec4 vertColor;
				
				void main(){
					fragmentColor = vertColor;
				}
			)";

	void processInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	void true_main()
	{
		int width = 800;
		int height = 600;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		float vertices[] = {
			//positions					//color1					//color2		 		//color3			
			0.5f, -0.5f, 0.0f,			1.0f, 0.0f, 0.0f,			0.0f, 1.0f, 0.0f,		0.0f, 0.0f, 1.0f,
			-0.5f, -0.5f, 0.0f,			0.0f, 1.0f, 0.0f,			0.0f, 0.0f, 1.0f,		1.0f, 0.0f, 0.0f,
			0.f, 0.5f, 0.0f,			0.0f, 0.0f, 1.0f, 			1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, 0.0f
		};
		unsigned int element_indices[] = {
			0, 1, 2,
		};


		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element_indices), element_indices, GL_STATIC_DRAW);

		//while these match a shader's configuration, we don't actually need an activer shader while specifying
		//because these vertex attribute configurations are associated with the VAO object, not with the compiled shader
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(9 * sizeof(float)));
		glEnableVertexAttribArray(3);

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //demonstrating that the vao also stores the state of the ebo

		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();

		while (!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			shader.use();
			float timeValue = static_cast<float>(glfwGetTime());
			float timeoffset = 2.0f;
			shader.setUniform1f("time", timeValue);
			shader.setUniform1f("timeoffset", timeoffset);
			std::cout << timeValue << std::endl;

			glBindVertexArray(vao);

			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); //binding the VAO also stores this state!
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));


			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
}

//int main()
//{
//	true_main();
//}