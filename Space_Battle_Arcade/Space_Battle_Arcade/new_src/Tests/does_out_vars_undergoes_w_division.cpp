#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>


namespace
{
	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				out INTERFACE_BLOCK {
					vec4 outPoint;
					vec4 color;
				} vs_out;

				void main(){
					float w = 10;
					gl_Position = vec4(position, w);
					vs_out.outPoint = vec4(position, w);
					vs_out.color = vec4(1, 1, 1, w);
				}
			)";

	const char* geometry_shader_src = R"(
				#version 330 core
				
				layout (triangles) in;
				layout (triangle_strip, max_vertices=6) out;

				in INTERFACE_BLOCK {
					vec4 outPoint;
					vec4 color;
				} vs_output[];

				out vec4 fragColor;				


				void main(){
					fragColor = vs_output[0].color;

					//this is the actual interpolated point
					/*
					gl_Position = gl_in[0].gl_Position;
					EmitVertex();

					gl_Position = gl_in[1].gl_Position;
					EmitVertex();

					gl_Position = gl_in[2].gl_Position;
					EmitVertex();
					EndPrimitive();
					*/

					//since this matches the above image (when it isn't commented out), it demonstrates that the
					//perspective division appears to happen after geometry shader based on the mesh_output in renderdoc of this file.
					gl_Position = vs_output[0].outPoint;
					EmitVertex();

					gl_Position = vs_output[1].outPoint;
					EmitVertex();

					gl_Position = vs_output[2].outPoint;
					EmitVertex();
					EndPrimitive();
				}
			)";

	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec4 fragColor;

				void main(){
					//this color is full white, so it appears that perspective division doesn't happen for out variables. Only primitives output by geometry shader.
					fragmentColor = fragColor;
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
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.5f, 0.5f, 0.0f,

			-0.5f, 0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f,
			0.5f, 0.5f, 0.0f
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

		GLuint geoShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geoShader, 1, &geometry_shader_src, nullptr);
		glCompileShader(geoShader);
		verifyShaderCompiled("geometry shader", geoShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &frag_shader_src, nullptr);
		glCompileShader(fragShader);
		verifyShaderCompiled("fragment shader", fragShader);

		GLuint shaderProg = glCreateProgram();
		glAttachShader(shaderProg, vertShader);
		glAttachShader(shaderProg, geoShader);
		glAttachShader(shaderProg, fragShader);
		glLinkProgram(shaderProg);
		verifyShaderLink(shaderProg);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glDeleteShader(geoShader);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		while (!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(shaderProg);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);


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