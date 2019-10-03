#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>


namespace
{
	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				

				uniform vec3 offset;
				uniform vec3 dir;
				uniform float time;

				void main(){
					gl_Position = vec4(position + offset, 1);
					gl_Position += (1.01 * sin(time)) * vec4(dir, 0.f);
					
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform float time;

				void main(){
					float baseIntensity = 0.1f;
					vec3 outColor = vec3(0.f);

					outColor.r = baseIntensity + sin(time);
					outColor.g = baseIntensity;
					outColor.b = baseIntensity + -sin(time);

					fragmentColor = vec4(outColor, 1.0f);
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
			exit(-1);
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
			exit(-1);
		}

	}

	void true_main()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(500, 500, "OpenGL Window", nullptr, nullptr);
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

		glViewport(0, 0, 500, 500);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		float vertices[] = {
			-0.25f, -0.25f, 0.0f,
			0.25f, -0.25f, 0.0f,
			0.0f, 0.25f, 0.0f
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		while (!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0.f, 0.f, 0.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(shaderProg);
			glBindVertexArray(vao);

			GLint offsetLoc = glGetUniformLocation(shaderProg, "offset");
			GLint timeLoc = glGetUniformLocation(shaderProg, "time");
			GLint dirLoc = glGetUniformLocation(shaderProg, "dir");

			glUniform1f(timeLoc, static_cast<float>(glfwGetTime()));

			//draw horizontal triangle
			glUniform3f(offsetLoc, 0.f, 0.5f, 0.f);
			glUniform3f(dirLoc, 1.f, 0.f, 0.f);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			//draw vertical triangle
			glUniform3f(offsetLoc, 0.f, 0.f, 0.f);
			glUniform3f(dirLoc, 0.f, 1.f, 0.f);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			//draw depth triangle
			glUniform3f(offsetLoc, 0.25f, -0.25f, 0.f);
			glUniform3f(dirLoc, 0.f, 0.f, 1.f);
			glDrawArrays(GL_TRIANGLES, 0, 3);


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