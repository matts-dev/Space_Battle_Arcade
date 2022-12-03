
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

				void main(){
					fragmentColor = vec4(0.1, 1, 0.1, 1);
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
				
				void main(){
					fragmentColor = texture(screencapture, texCoord);
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
		int samples = 4;
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		//-------------- glfw needs to know if we want to set up default MSAA -------------------
		//glfwWindowHint(GLFW_SAMPLES, samples);
		//---------------------------------------------------------------------------------------
		
		GLFWwindow* window = glfwCreateWindow(width, height, "OpenglContext", nullptr, nullptr);
		if (!window)
		{
			std::cerr << "failed to create window" << std::endl;
			exit(-1);
		}
		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << "failed to initialize glad with processes " << std::endl;
			exit(-1);
		}
		//------------------------------------ enable opengl multisampling -------------------------------------
		//glEnable(GL_MULTISAMPLE);
		//-----------------------------------------------------------------------------------------------
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
			-0.5f,  0.5f, -0.5f
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

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);

		Shader postprocessShader(postprocess_vertex_shader_src, postprocess_frag_shader_src, false);
		postprocessShader.use();
		postprocessShader.setUniform1i("screencapture", 0);


		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();

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

		// -------------------------------------- MSAA FRAME BUFFER SETUP-------------------------------------------------------------
		GLuint msaaFB;
		glGenFramebuffers(1, &msaaFB);
		glBindFramebuffer(GL_FRAMEBUFFER, msaaFB); //bind both read/write to the target framebuffer

		GLuint texMutiSampleColor;
		glGenTextures(1, &texMutiSampleColor);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texMutiSampleColor);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texMutiSampleColor, 0);

		GLuint rendbufferMSAADepthStencil;
		glGenRenderbuffers(1, &rendbufferMSAADepthStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, rendbufferMSAADepthStencil);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0); //unbind the render buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rendbufferMSAADepthStencil);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "error on setup of framebuffer" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //bind to default frame buffer

		// -------------------------------------- END FRAME BUFFER -------------------------------------------------------------
		// -------------------------------------- SAMPLE RESOLVING FB-------------------------------------------------------------
		GLuint resolvingFB;
		glGenFramebuffers(1, &resolvingFB);
		glBindFramebuffer(GL_FRAMEBUFFER, resolvingFB);

		GLuint screenTex;
		glGenTextures(1, &screenTex);
		glBindTexture(GL_TEXTURE_2D, screenTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTex, 0); 

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "error on setup of framebuffer" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //bind to default frame buffer

		// -------------------------------------- END RESOLVING FB-------------------------------------------------------------

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glBindFramebuffer(GL_FRAMEBUFFER, msaaFB);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			shader.use();
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

			glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFB);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvingFB);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			//enable default framebuffer object
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST); //don't let depth buffer disrupt drawing (although this doesn't seem to affect setup)

			postprocessShader.use();
			glBindVertexArray(postVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, screenTex);
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
		glDeleteFramebuffers(1, &msaaFB);
		glDeleteTextures(1, &texMutiSampleColor);
		glDeleteRenderbuffers(1, &rendbufferMSAADepthStencil);
	}
}

//int main()
//{
//	true_main();
//}