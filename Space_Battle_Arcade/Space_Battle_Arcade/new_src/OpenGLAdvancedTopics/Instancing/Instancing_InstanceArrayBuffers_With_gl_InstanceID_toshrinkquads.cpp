
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace
{
	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec2 position;				
				layout (location = 1) in vec3 color;				
				layout (location = 2) in vec2 offset;				

				out vec3 fragColor;

				void main(){
					vec2 scaledPos = position * (gl_InstanceID / 100.0);
					gl_Position = vec4(scaledPos + offset, 0 , 1);
					fragColor = color;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec3 fragColor;

				void main(){
					fragmentColor = vec4(fragColor, 1.0f);
				}
			)";

	void processInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	void verifyShaderCompiled(const char* shadername, GLuint shader)
	{
		GLint success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			constexpr int size = 512;
			char infolog[size];

			glGetShaderInfoLog(shader, size, nullptr, infolog);
			std::cerr << "shader failed to compile: " << shadername << infolog << std::endl;
		}
	}

	void verifyShaderLink(GLuint shaderProgram)
	{
		GLint success = 0;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success)
		{
			constexpr int size = 512;
			char infolog[size];

			glGetProgramInfoLog(shaderProgram, size, nullptr, infolog);
			std::cerr << "SHADER LINK ERROR: " << infolog << std::endl;
		}

	}

	void true_main()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(800, 600, "OpenglContext", nullptr, nullptr);
		if (!window)
		{
			std::cerr << "failed to create window" << std::endl;
			return;
		}
		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << "failed to initialize glad with processes " << std::endl;
			return;
		}

		glViewport(0, 0, 800, 600);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		float vertices[] = {
			// positions     // colors
			-0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
			0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
			-0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

			-0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
			0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
			0.05f,  0.05f,  0.0f, 1.0f, 1.0f
		};

		//instanced positions
		glm::vec2 instanceOffsets[100];
		float offsetBase = 0.1f;
		int idx = 0;
		for (int y = -10; y < 10; y += 2)
		{
			for (int x = -10; x < 10; x += 2)
			{
				instanceOffsets[idx].x = offsetBase + static_cast<float>(x) / 10;
				instanceOffsets[idx].y = offsetBase + static_cast<float>(y) / 10;
				idx++;
			}
		}

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// ---------------------------------- setting up instance array buffer
		GLuint vboInstanceOffsets;
		glGenBuffers(1, &vboInstanceOffsets);
		glBindBuffer(GL_ARRAY_BUFFER, vboInstanceOffsets);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, instanceOffsets, GL_STATIC_DRAW);		//perhaps stream or dynamic draw?
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1); //sets vertex attribute to update after every 1 instance drawn. This way we get a new offset for each instance.
		 //glBindBuffer(GL_ARRAY_BUFFER, 0);

		 //The following call seems kind of weird to me.
		 //		""" glVertexAttribDivisor(2, 1); """
		 // Here's a way to to remember what it does:
		 //
		 //	The default divisor is 0. This means the vertex attribute updates after 0 instances are drawn...
		 //  So the first time we read a attribute in the shader, it is for the first vertex.
		 //	The next time we read the attribute, we have drawn 0 instances, so we should update it! Thus the second vertex gets a different vertex attribute.
		 // 
		 // Now, when we set the vertex attribute divisor to be 1, we need to update it after we render 1 complete instance.
		 // So the first vertex gets the first value in the vertex attribute! 
		 // When the second vertex enters the vertex shader, we still haven't drawn another instance. So this attribute doesn't meet the criteria of 1...
		 //		which means we keep this the same for the second vertex! It is only after we have rendered all vertices in an instance, does this attribut get updtaed via the stride
		 // -------------------------------------------------------------------

		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &vertex_shader_src, nullptr);
		glCompileShader(vertShader);
		verifyShaderCompiled("vertex shader", vertShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &frag_shader_src, nullptr);
		glCompileShader(fragShader);
		verifyShaderCompiled("fragment shader", fragShader);

		GLuint shaderProg = glCreateProgram();
		glAttachShader(shaderProg, vertShader);
		glAttachShader(shaderProg, fragShader);
		glLinkProgram(shaderProg);
		verifyShaderLink(shaderProg);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);

		//no longer needed with instance arrays!
		//glUseProgram(shaderProg);
		//for (int i = 0; i < 100; ++i)
		//{
		//	GLuint offsetLoc = glGetUniformLocation(shaderProg, ("offsets[" + std::to_string(i) + "]").c_str());
		//	glUniform2f(offsetLoc, instanceOffsets[i].x, instanceOffsets[i].y);
		//}

		while (!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(shaderProg);
			glBindVertexArray(vao);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);

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