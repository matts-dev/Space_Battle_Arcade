//Create two shader programs where the second program uses a different fragment shader 
//that outputs the color yellow; draw both triangles again where one outputs the color yellow 
#include<iostream>
#include<cstdint>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

namespace HTChallenge3 {

	const int HEIGHT = 400;
	const int WIDTH = 400;
	enum class Color : int8_t {ORANGE, YELLOW};
	GLFWwindow* startOpenGL();
	GLuint createdLinkedShaderProgram(Color color);
	bool createTriangle(GLuint& VAO, GLuint& VBO, float* verticesArray, size_t size);
	void pollInput(GLFWwindow* window);

	int main() {
		GLFWwindow* window = startOpenGL();
		if (!window) return -1;

		GLuint linkedOrangeShader = createdLinkedShaderProgram(Color::ORANGE);
		GLuint linkedYellowShader = createdLinkedShaderProgram(Color::YELLOW);

		bool success = true;
		float vertices1[] = {/*1*/ 0.f, -0.75f, 0.0f, /*2*/ 0.f, 0.75f, 0.0f, /*3*/ -0.5f,  -0.0f, 0.0f };
		float vertices2[] = { /* 1 */ 0.f, -0.75f, 0.0f, /* 2 */ 0.f, 0.75f, 0.0f, /* 3 */  0.5f,  -0.0f, 0.0f };
		GLuint VAOs[2];
		GLuint VBOs[2];
		success &= createTriangle(VAOs[0], VBOs[0], vertices1, sizeof(vertices1));
		success &= createTriangle(VAOs[1], VBOs[1], vertices2, sizeof(vertices2));
		if (!success) { glfwTerminate(); return -1; }


		while (!glfwWindowShouldClose(window)) {

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			//actually draw triangles
			glUseProgram(linkedOrangeShader);
			glBindVertexArray(VAOs[0]);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glUseProgram(linkedYellowShader);
			glBindVertexArray(VAOs[1]);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glfwSwapBuffers(window);
			glfwPollEvents();
			pollInput(window);
		}

		glDeleteVertexArrays(2, VAOs);
		glDeleteBuffers(2, VBOs);

		glfwTerminate();
		return 0;
	}


// ---------------------------------------------------------------- //
	GLFWwindow * startOpenGL()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_COMPAT_PROFILE, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Challenge 3", nullptr, nullptr);
		if (!window) { glfwTerminate(); return nullptr; }

		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
			std::cerr << "failed to load glad" << std::endl;
			glfwTerminate(); return nullptr;
		}

		glViewport(0, 0, WIDTH, HEIGHT);

		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {glViewport(0, 0, width, height); }); //quick lambda for call back, reality should probably make a function

		return window;
	}

	GLuint createdLinkedShaderProgram(Color color)
	{
		static const char* vertexShaderSource =
			"#version 330 core\n"
			"layout (location = 0) in vec3 aPos;\n"
			"void main()\n"
			"{\n"
			"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
			"}\n\0";

		static const char* orangeFragmentShaderSource =
			"#version 330 core\n"
			"out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"    //always render an orange color\n"
			"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
			"}\n\0";

		static const char* yellowFragmentShaderSource =
			"#version 330 core\n"
			"out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"    //always render an yellowish color\n"
			"    FragColor = vec4(1.0f, 0.5f, 0.5f, 1.0f);\n"
			"}\n\0";

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glShaderSource(fragShader, 1, color == Color::ORANGE ? &orangeFragmentShaderSource : &yellowFragmentShaderSource, nullptr);

		char infoLog[256];
		int success = 0;

		glCompileShader(vertexShader);
		glCompileShader(fragShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 256, 0, infoLog);
			std::cerr << "failed to compile vertex shader\n" << infoLog << std::endl;
			glfwTerminate(); return 0;
		}

		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragShader, 256, nullptr, infoLog);
			std::cerr << "failed to compile vertex shader\n" << infoLog << std::endl;
			glfwTerminate(); return 0;
		}

		GLuint linkedProgram = glCreateProgram();
		glAttachShader(linkedProgram, vertexShader);
		glAttachShader(linkedProgram, fragShader);

		glLinkProgram(linkedProgram);
		glGetProgramiv(linkedProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(linkedProgram, 256, nullptr, infoLog);
			std::cerr << "failed to link shader\n" << infoLog << std::endl;
			glfwTerminate(); return 0;
		}

		return linkedProgram;
	}

	bool createTriangle(GLuint & VAO, GLuint & VBO, float * verticesArray, size_t size)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		if (!VAO || !VBO) { glfwTerminate(); return false; }

		glBindVertexArray(VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, size, verticesArray, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
		return true;
	}

	void pollInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

}

//int main() {
//	return HTChallenge3::main();
//}