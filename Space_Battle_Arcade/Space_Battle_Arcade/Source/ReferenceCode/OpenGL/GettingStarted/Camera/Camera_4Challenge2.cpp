#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"

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

		glEnable(GL_DEPTH_TEST);

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

			//--------------------- construct a camera view matrix ----------------------------------
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
			//construct camera basis vectors  (u, v, w) ~~ (x, y, z)
			glm::vec3 cameraAxisW = glm::normalize(cameraPosition - (cameraFront + cameraPosition)); //points towards camera; camera looks down its -z axis (ie down its -w axis)
			glm::vec3 cameraAxisU = glm::normalize(glm::cross(worldUp, cameraAxisW)); //this is the right axis (ie x-axis)
			glm::vec3 cameraAxisV = glm::normalize(glm::cross(cameraAxisW, cameraAxisU));
			//make each row in the matrix a camera's basis vector; glm is column-major
			glm::mat4 cameraBasisProjection;
			cameraBasisProjection[0][0] = cameraAxisU.x;
			cameraBasisProjection[1][0] = cameraAxisU.y;
			cameraBasisProjection[2][0] = cameraAxisU.z;

			cameraBasisProjection[0][1] = cameraAxisV.x;
			cameraBasisProjection[1][1] = cameraAxisV.y;
			cameraBasisProjection[2][1] = cameraAxisV.z;

			cameraBasisProjection[0][2] = cameraAxisW.x;
			cameraBasisProjection[1][2] = cameraAxisW.y;
			cameraBasisProjection[2][2] = cameraAxisW.z;
			glm::mat4 cameraTranslate = glm::translate(glm::mat4(), -1.f * cameraPosition);

			glm::mat4 view = cameraBasisProjection * cameraTranslate;
			//glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, worldUp);


			//--------------------- construct a camera view matrix ----------------------------------


			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				glm::mat4 model;
				float angle = 20.0f * i;
				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

				//author says its best practice to not re-initialize this everytime since it rarely changes; leaving here for close proximity to other matrices
				glm::mat4 projection;
				projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);

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