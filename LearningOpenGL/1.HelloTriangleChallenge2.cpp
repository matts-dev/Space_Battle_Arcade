//2. Now create the same 2 triangles using two different VAOs and VBOs for their data:
//I'm recreating this from scratch to get extra practice with all aspects of initializing openGL
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#define ch2_height 600
#define ch2_width 800


namespace HT_Challenge2 {
	GLFWwindow* initializeOpenGL();
	void handleInput(GLFWwindow* window);
	GLuint generateShaders();
	bool createTriangle(GLuint& VAO, GLuint& VBO, float* vertices, GLuint size);

	int main() {
		GLFWwindow* window = initializeOpenGL();
		if (!window) return -1;

		GLuint shaderProgram = generateShaders();
		if (!shaderProgram) { return -1; glfwTerminate(); }

		//triangle points
		float vertices1[] = {/*1*/ 0.f, -0.75f, 0.0f, /*2*/ 0.f, 0.75f, 0.0f, /*3*/ -0.5f,  -0.0f, 0.0f };
		float vertices2[] = { /* 1 */ 0.f, -0.75f, 0.0f, /* 2 */ 0.f, 0.75f, 0.0f, /* 3 */  0.5f,  -0.0f, 0.0f };
		GLuint VAOs[2];
		GLuint VBOs[2];

		//create triangles
		if(!createTriangle(VAOs[0], VBOs[0], vertices1, sizeof(vertices1))) { glfwTerminate(); return -1; }
		if(!createTriangle(VAOs[1], VBOs[1], vertices2, sizeof(vertices2))) { glfwTerminate(); return -1; }

		while (!glfwWindowShouldClose(window)) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(shaderProgram);
			glBindVertexArray(VAOs[0]);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glBindVertexArray(VAOs[1]);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glBindVertexArray(0);
			handleInput(window);
			glfwPollEvents();
			glfwSwapBuffers(window);
		}

		glfwTerminate();
		glDeleteBuffers(2, VBOs);
		glDeleteVertexArrays(2, VAOs);
		return 0;
	}

	// ----------------- Helper Function Definitions ------------------ //
	GLFWwindow* initializeOpenGL() {
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_COMPAT_PROFILE, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(ch2_width, ch2_height, "Challenge 2!", nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			return nullptr;
		}
		glfwMakeContextCurrent(window);

		//test that glad loaded
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
			glfwTerminate();
			return nullptr;
		}

		glViewport(0, 0, ch2_width, ch2_height);

		//instead of defining a callback, just use lambda 
		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {glViewport(0, 0, width, height); });

		return window;
	}

	void handleInput(GLFWwindow * window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
	GLuint generateShaders()
	{
		static const char* vertexShaderSource =
			"#version 330 core\n"
			"layout (location = 0) in vec3 aPos;\n"
			"void main()\n"
			"{\n"
			"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
			"}\n\0";

		static const char* fragmentShaderSource =
			"#version 330 core\n"
			"out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"    //always render an orange color\n"
			"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
			"}\n\0";

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glShaderSource(fragShader, 1, &fragmentShaderSource, nullptr);

		glCompileShader(vertexShader);
		glCompileShader(fragShader);

		char infolog[256];
		int success = 0;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertexShader, 256, 0, infolog);
			std::cerr << "Vertex shader failed to compile\n" << infolog << std::endl;
			return 0;
		}
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragShader, 256, 0, infolog);
			std::cerr << "frag shader failed to compile\n" << infolog << std::endl;
			return 0;
		}

		GLuint linkedProgram = glCreateProgram();
		glAttachShader(linkedProgram, vertexShader);
		glAttachShader(linkedProgram, fragShader);
		glLinkProgram(linkedProgram);

		glGetProgramiv(linkedProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(linkedProgram, 256, 0, infolog);
			std::cerr << "failed to link shaders into program\n" << infolog << std::endl;
			return 0;
		}
		
		return linkedProgram;
	}

	bool createTriangle(GLuint & VAO, GLuint & VBO, float * vertices, GLuint verticesSize)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		if (!VAO || !VBO) return false;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
		return true;
	}
}

int main() {
	HT_Challenge2::main();
}