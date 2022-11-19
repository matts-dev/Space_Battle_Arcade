#pragma once
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

	const char* postprocess_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec2 position;				
				layout (location = 1) in vec2 inTexCoord;
				
				out vec2 texCoord;

				void main(){
					texCoord = inTexCoord;
					gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
				}
			)";
	const char* postprocess_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 texCoord;

				uniform sampler2D screencapture;

				//radius of kernel effect (large denominator is smaler radius)
				const float offset = 1 / 300.0f;
				
				void main(){

					//notice strange GLSL array initialization syntax (looks like a ctor); glsl doesn't support c-style initializers
					vec2 offsets[9] = vec2[](
						vec2(-offset, offset),	//TL
						vec2(0,	offset),		//TC
						vec2(offset, offset),	//TR
						vec2(-offset, 0),		//ML
						vec2(0, 0),				//center
						vec2(offset, 0),		//MR
						vec2(-offset, -offset),	//BL
						vec2(0, -offset	),		//BC
						vec2(offset, -offset)	//BR
					);

					float kernel[] = float[](
						1, 1,  1,
						1, -8, 1,
						1, 1,  1
					);
					
					vec3 sampledTexture[9];
					for(int i = 0; i < 9; ++i){
						sampledTexture[i] = vec3(texture(screencapture, texCoord.st + offsets[i]));
					}					

					vec3 color = vec3(0.0f);
					for(int i = 0; i < 9; ++i){
						color += sampledTexture[i] * kernel[i];
					}
					fragmentColor = vec4(color, 1.0f);
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

		float quadVerts[] = {
			//x    y         s    t
			-1.0, -1.0,		0.0, 0.0,
			-1.0, 1.0,		0.0, 1.0,
			1.0, -1.0,		1.0, 0.0,

			1.0, -1.0,      1.0, 0.0,
			-1.0, 1.0,      0.0, 1.0,
			1.0, 1.0,		1.0, 1.0
		};

		//------------------------ postprocess quad --------------------------------------------------
		GLuint postVAO;
		glGenVertexArrays(1, &postVAO);
		glBindVertexArray(postVAO);

		GLuint postVBO;
		glGenBuffers(1, &postVBO);
		glBindBuffer(GL_ARRAY_BUFFER, postVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		// -------------------------- cubes ---------------------------------------
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


		Shader postprocessShader(postprocess_vertex_shader_src, postprocess_frag_shader_src, false);
		postprocessShader.use();
		postprocessShader.setUniform1i("screencapture", 0);


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
			glm::vec3(3.f,  0.0f, 0.0f)
		};

		// -------------------------------------- FRAME BUFFER SETUP-------------------------------------------------------------
		GLuint framebufferObject;
		glGenFramebuffers(1, &framebufferObject);

		glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject); //bind both read/write to the target framebuffer
															  ///glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferObject); //this allows us to bind to the specific framebuffer to write to (but not read) {eg glReadPixels}
															  ///glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferObject); //we can also specifically bind to a frame buffer to read from (I guess for depth/stencil testing) {eg rendering, clearing, otherWriteOperations}

		GLuint fbo_Texture_ColorAttachment;
		glGenTextures(1, &fbo_Texture_ColorAttachment);
		glBindTexture(GL_TEXTURE_2D, fbo_Texture_ColorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_INT, nullptr); //pass null to texture data, we only want to allocate memory -- not fill it.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //fix for kernel on edge of screen causing top of screen to bleed to bottom
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); //fix for kernel on edge of screen causing top of screen to bleed to bottom
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_Texture_ColorAttachment, 0); //last argument is mipmap level

																													 //option A: use plain-old textures for depth and stencil buffers
		GLuint fbo_Texture_DepthStencilTexture; //this is actually less performant than using a renderbuffer (it is in opengl's native dataformat). 
		glGenTextures(1, &fbo_Texture_DepthStencilTexture);
		glBindBuffer(GL_TEXTURE_2D, fbo_Texture_DepthStencilTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbo_Texture_DepthStencilTexture, 0);
		//you can also attach separate depth/stencil buffers with GL_DEPTH_ATTACHEMENT (texture format GL_DEPTH_COMPONENT_ and GL_STENCIL_ATTACHMENT (texture format is GL_STENCIL_INDEX)

		//option B: use the faster renderbuffers (OpenGL native render format) for depth and stencil buffers (these cannot easily be read from, but can be used in depth/stenci tests)
		GLuint fbo_RenderBufferObject_DepthStencil;
		glGenRenderbuffers(1, &fbo_RenderBufferObject_DepthStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, fbo_RenderBufferObject_DepthStencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo_RenderBufferObject_DepthStencil); //this overwrites what we set up in option A
		glBindRenderbuffer(GL_RENDERBUFFER, 0); //unbind the render buffer

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "error on setup of framebuffer" << std::endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0); //bind to default frame buffer
											  // -------------------------------------- END FRAME BUFFER -------------------------------------------------------------

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);

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
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);

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

			//enable default framebuffer object
			postprocessShader.use();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST); //don't let depth buffer disrupt drawing (although this doesn't seem to affect setup)

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbo_Texture_ColorAttachment);

			glBindVertexArray(postVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glEnable(GL_DEPTH_TEST); //don't let depth buffer disrupt drawing

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);

		glDeleteVertexArrays(1, &postVAO);
		glDeleteBuffers(1, &postVBO);

		//framebuffer related
		glDeleteFramebuffers(1, &framebufferObject);
		glDeleteTextures(1, &fbo_Texture_ColorAttachment);
		glDeleteTextures(1, &fbo_Texture_DepthStencilTexture);
		glDeleteRenderbuffers(1, &fbo_RenderBufferObject_DepthStencil);
	}
}

//int main()
//{
//	true_main();
//}