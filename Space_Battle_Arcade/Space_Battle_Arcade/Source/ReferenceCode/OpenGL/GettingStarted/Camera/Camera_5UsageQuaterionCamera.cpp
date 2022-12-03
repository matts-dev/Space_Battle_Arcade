#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "CameraQuaternionFPS.h"
#include "CameraQuaternionExperiment1Flawed.h"
#include "FreeCamera.h"

namespace
{
	FreeCamera camera(45.0f);

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
		int img_width, img_height, img_nrChannels;
		unsigned char *textureData = stbi_load("Textures/container.jpg", &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << std::endl;
			exit(-1);
		}
		stbi_set_flip_vertically_on_load(true);

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
			glm::vec3(0.f,  0.2f, 6.0f),
			glm::vec3(3.f,  0.0f, 3.0f)
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

			glm::mat4 view = camera.getView();

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				glm::mat4 model;
				float angle = 20.0f * i;
				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

				//use the camera's view!
				//glm::mat4 view;
				//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

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
//
//int main()
//{
//	true_main();
//}