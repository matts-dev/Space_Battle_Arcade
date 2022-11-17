#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../nu_utils.h"
#include "../../Shader.h"
#include "../../Libraries/stb_image.h"
#include "../../InputTracker.h"

namespace
{
	glm::vec3 cameraPosition(0.f, 0.f, 3.0f);
	glm::vec3 cameraFront(0.f, 0.f, -1.f);
	glm::vec3 worldUp(0.f, 1.f, 0.f);

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
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					texCoord = inTexCoord;
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 texCoord;

				uniform sampler2D texture0;
				uniform sampler2D texture1;
				
				uniform float near;
				uniform float far;
				
				void main(){
					
					fragmentColor = mix(
							texture(texture0, texCoord),
							texture(texture1, texCoord),
							0.2);
					
					//values are close to white
					//fragmentColor = vec4(vec3(gl_FragCoord.z), 1);

					//tutorial's method
					//float depth = gl_FragCoord.z; //renaming for readability
					//float z = depth * 2.0f - 1.0f;
					//float linearDepth = (2.0 * near * far) / (far + near - z * (far - near));
					//linearDepth = linearDepth / far;
					//fragmentColor = vec4(vec3(linearDepth), 1.0f);

					//my method based on algebra of equation he gave; probably a mistake here.
					//float depth = gl_FragCoord.z;
					//float z = (far*near)/(depth*(near-far)) + (near*depth*(near-far))/(depth*(near-far));
					//fragmentColor = vec4(vec3(z), 1.0f);
				}
			)";
	bool depthFuncSetLesser = true;
	void processInput(GLFWwindow* window)
	{
		static InputTracker input;
		input.updateState(window);

		if (input.isKeyJustPressed(window, GLFW_KEY_M))
		{
			static bool depthMaskValue = true;
			depthMaskValue = !depthMaskValue;

			glDepthMask(depthMaskValue ? GL_TRUE : GL_FALSE);
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			depthFuncSetLesser = !depthFuncSetLesser;

			//toggle between lesser/greater depth tests
			glDepthFunc(depthFuncSetLesser ? GL_LESS : GL_GREATER);
		}

		float cameraSpeed = 2.5f * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			cameraPosition -= cameraFront * cameraSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			cameraPosition += cameraFront * cameraSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, -cameraFront));
			cameraPosition += cameraRight * cameraSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, -cameraFront));
			cameraPosition -= cameraRight * cameraSpeed;
		}
	}

	double lastX = 400;
	double lastY = 400;
	bool refocused = true;
	void mouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (refocused)
		{
			lastX = xpos;
			lastY = ypos;
			refocused = false;
		}

		double xOff = xpos - lastX;
		double yOff = lastY - ypos;

		float sensitivity = 0.05f;
		xOff *= sensitivity;
		yOff *= sensitivity;

		yaw += static_cast<float>(xOff);
		pitch += static_cast<float>(yOff);

		lastX = xpos;
		lastY = ypos;

		if (pitch > 89.0f)
			pitch = 89.f;
		else if (pitch < -89.0f)
			pitch = -89.f;

		glm::vec3 camDir;
		camDir.y = sin(glm::radians(pitch));
		camDir.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		camDir.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		camDir = glm::normalize(camDir); //perhaps unnecessary
		cameraFront = camDir;
	}

	void mouseLeftCallback(GLFWwindow* window, int Enter)
	{
		if (Enter != GLFW_TRUE)
		{
			refocused = true;
			//std::cout << "exit" << std::endl;
		}
		else
		{
			//std::cout << "entered" << std::endl;
		}
	}

	void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		if (FOV >= 1.0f && FOV <= 45.0f)
			FOV -= static_cast<float>(yOffset);
		if (FOV <= 1.0f)
			FOV = 1.0f;
		if (FOV >= 45.0)
			FOV = 45.0f;
	}

	void true_main()
	{
		int width = 800;
		int height = 600;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		stbi_set_flip_vertically_on_load(true);

		//callbacks
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetCursorEnterCallback(window, &mouseLeftCallback);
		glfwSetScrollCallback(window, &scrollCallback);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		
		//LOAD BACKGROUND TEXTURE
		int img_width, img_height, img_nrChannels;
		unsigned char *textureData = stbi_load("Textures/container.jpg", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
			exit(-1);
		}
		GLuint textureBackgroundId;
		glGenTextures(1, &textureBackgroundId);
		glActiveTexture(GL_TEXTURE0); //set this to the first texture unit
		glBindTexture(GL_TEXTURE_2D, textureBackgroundId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		stbi_image_free(textureData);

		//LOAD FACE TEXTURE
		textureData = stbi_load("Textures/awesomeface.png", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
			exit(-1);
		}
		GLuint textureFaceId;
		glGenTextures(1, &textureFaceId);
		glActiveTexture(GL_TEXTURE0 + 1); //2nd texture unit
		glBindTexture(GL_TEXTURE_2D, textureFaceId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		stbi_image_free(textureData);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureBackgroundId);

		//set texture unit 1 to be face
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, textureFaceId);
		

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

		//while these match a shader's configuration, we don't actually need an active shader while specifying
		//because these vertex attribute configurations are associated with the VAO object, not with the compiled shader
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //demonstrating that the vao also stores the state of the ebo

		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();

		//inform shader which texture units that its samplers should be bound to
		shader.setUniform1i("texture0", 0); //binds sampler "texture0" to texture unit GL_TEXTURE0
		shader.setUniform1i("texture1", 1); // "												"


		glm::vec3 cubePositions[] = {
			glm::vec3(0.0f,  0.0f,  0.0f),
			glm::vec3(2.0f,  5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3(2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f,  3.0f, -7.5f),
			glm::vec3(1.3f, -2.0f, -2.5f),
			glm::vec3(1.5f,  2.0f, -2.5f),
			glm::vec3(1.5f,  0.2f, -1.5f),
			glm::vec3(-1.3f,  1.0f, -1.5f)
		};

		//camera
		glm::vec3 cameraPosDemo(0.f, 0.f, 3.f);
		glm::vec3 targetDemo(0.f, 0.f, 0.f);
		glm::vec3 worldUpDemo(0.f, 1.f, 0.f);

		//construct camera basis vectors  (u, v, w) ~~ (x, y, z)
		glm::vec3 cameraWaxis = glm::normalize(cameraPosDemo - targetDemo); //points towards camera; camera looks down its -z axis (ie down its -w axis)
		glm::vec3 cameraUaxis = glm::normalize(glm::cross(worldUpDemo, cameraWaxis)); //this is the right axis (ie x-axis)
		glm::vec3 cameraVaxis = glm::normalize(glm::cross(cameraWaxis, cameraUaxis));

		//this creates a view matrix from two matrices.
		//the first matrix (ie the matrix on the right which gets applied first) is just translating points by the opposite of the camera's position.
		//the second matrix (ie matrix on the left) is a matrix where the first 3 rows are the U, V, and W axes of the camera.
		//		The matrix with the axes of the camera is a change of basis matrix.
		//		If you think about it, what it does to a vector is a dot product with each of the camera's axes.
		//		Since the camera axes are all normalized, what this is really doing is projecting the vector on the 3 axes of the camera!
		//			(recall that a vector dotted with a normal vector is equivalent of getting the scalar projection onto the normal)
		//		So, the result of the projection is the vector's position in terms of the camera position.
		//		Since we're saying that the camera's axes align with our NDC axes, we get coordinates in terms x y z that are ready for clipping and projection to NDC 
		//So, ultimately the lookAt matrix shifts points based on camera's position, then projects them onto the camera's axes -- which are aligned with OpenGL's axes 
		//
		//This all relies on the fact that we say the camera axes match the final axes before when we're done processing
		glm::mat4 viewDemo = glm::lookAt(cameraPosDemo, targetDemo, worldUpDemo);

		glEnable(GL_DEPTH_TEST);
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			//like clear color, but for the depth buffer
			if (!depthFuncSetLesser)
				glClearDepth(-1.f);
			else
				glClearDepth(1.f);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			shader.use();

			//set texture unit 0 to be the background
			//glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, textureBackgroundId);

			//set texture unit 1 to be face
			//glActiveTexture(GL_TEXTURE0 + 1);
			//glBindTexture(GL_TEXTURE_2D, textureFaceId);

			float farDepth = 100.0f;
			float nearDepth = 0.1f;

			shader.setUniform1f("far", farDepth);
			shader.setUniform1f("near", nearDepth);

			glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, worldUp);
			glm::mat4 projection;
			projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, nearDepth, farDepth);

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				glm::mat4 model;
				float angle = 20.0f * i;
				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));


				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

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