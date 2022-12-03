

#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include <map>

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
					texCoord = inTexCoord; //reminder, since this is an out variable it will be subject to fragment interpolation (which is what we want)
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 texCoord;

				uniform sampler2D texture0;
				uniform sampler2D texture1;
				
				void main(){
					fragmentColor = mix(
							texture(texture0, texCoord),
							texture(texture1, texCoord),
							0.2);
				}
			)";

	const char* grass_vertex_shader_src = R"(
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
	const char* grass_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 texCoord;

				uniform sampler2D texture0;
				
				void main(){
					fragmentColor =	texture(texture0, texCoord);
				}
			)";

	void processInput(GLFWwindow* window)
	{
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

		//LOAD BACKGROUND TEXTURE
		int img_width, img_height, img_nrChannels;
		unsigned char *textureData = stbi_load("Textures/container.jpg", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
			exit(-1);
		}
		stbi_set_flip_vertically_on_load(true);

		//callbacks
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetCursorEnterCallback(window, &mouseLeftCallback);
		glfwSetScrollCallback(window, &scrollCallback);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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


		//LOAD WINDOW TEXTURE
		textureData = stbi_load("Textures/blending_transparent_window.png", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
			exit(-1);
		}
		GLuint textureWindowId;
		glGenTextures(1, &textureWindowId);
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, textureWindowId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		stbi_image_free(textureData);

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


		float planeVertices[] = {
			// xyz			       // st
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 1.0f
		};

		GLuint windowVAO;
		glGenVertexArrays(1, &windowVAO);
		glBindVertexArray(windowVAO);

		GLuint windowVBO;
		glGenBuffers(1, &windowVBO);
		glBindBuffer(GL_ARRAY_BUFFER, windowVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		Shader windowShader(grass_vertex_shader_src, grass_frag_shader_src, false);
		windowShader.use();
		windowShader.setUniform1i("texture0", 2); //bind sampler to 3rd texture slot

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

		std::vector<glm::vec3> windowPositions;
		windowPositions.push_back(glm::vec3(-1.5f, 0.0f, -0.48f));
		windowPositions.push_back(glm::vec3(1.5f, 0.0f, 0.51f));
		windowPositions.push_back(glm::vec3(0.0f, 0.0f, 0.7f));
		windowPositions.push_back(glm::vec3(-0.3f, 0.0f, -2.3f));
		windowPositions.push_back(glm::vec3(0.5f, 0.0f, -0.6f));

		//camera
		glm::vec3 cameraPosDemo(0.f, 0.f, 3.f);
		glm::vec3 targetDemo(0.f, 0.f, 0.f);
		glm::vec3 worldUpDemo(0.f, 1.f, 0.f);

		//construct camera basis vectors  (u, v, w) ~~ (x, y, z)
		glm::vec3 cameraWaxis = glm::normalize(cameraPosDemo - targetDemo); //points towards camera; camera looks down its -z axis (ie down its -w axis)
		glm::vec3 cameraUaxis = glm::normalize(glm::cross(worldUpDemo, cameraWaxis)); //this is the right axis (ie x-axis)
		glm::vec3 cameraVaxis = glm::normalize(glm::cross(cameraWaxis, cameraUaxis));

		std::map<float, glm::vec3> sortedPositions;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			shader.use();

			//set texture unit 0 to be the background
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureBackgroundId);

			//set texture unit 1 to be face
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, textureFaceId);

			glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, worldUp);
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				glm::mat4 model;
				float angle = 20.0f * i;
				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				//shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				//shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			sortedPositions.clear();
			for (const auto& pos : windowPositions)
			{
				//glm::length2 could be used here as an optimization for removing the sqrt; length2 is length_squared and that can be used to preserve order
				sortedPositions[glm::length(pos - cameraPosition)] = pos;
			}

			windowShader.use();
			glActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, textureWindowId);
			for (auto iter = sortedPositions.rbegin(); iter != sortedPositions.rend(); ++iter)
			{
				const glm::vec3& pos = iter->second;
				glm::mat4 model;
				model = glm::translate(model, pos);
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
				windowShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				windowShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				windowShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				glBindVertexArray(windowVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
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