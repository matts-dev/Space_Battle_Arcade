
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

namespace HTPractice4 {

	GLFWwindow* initOpenGL();
	GLuint generateLinkedShaderProgram();
	bool generateElement(GLuint& EAO, GLuint& VAO, GLuint& VBO);
	void processInputs(GLFWwindow* window);

	int main() {

		GLFWwindow* window = initOpenGL();
		if (!window) return -1;

		GLuint linkedShader = generateLinkedShaderProgram();

		GLuint EAO, VAO, VBO = 0;
		if (!generateElement(EAO, VAO, VBO)) { glfwTerminate(); return -1; }

		while (!glfwWindowShouldClose(window)) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			processInputs(window);
			glfwPollEvents();

			//draw the element!
			glUseProgram(linkedShader);
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glfwSwapBuffers(window);
		}

		glfwTerminate();
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &EAO);
		glDeleteBuffers(1, &VBO);
		return 0;
	}

	GLFWwindow * initOpenGL()
	{
		static const int WIDTH = 400;
		static const int HEIGHT = 400;

		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Using Elements!", nullptr, nullptr);
		if (!window) { glfwTerminate(); return nullptr; }

		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
			glfwTerminate();
			return nullptr;
		}

		glViewport(0, 0, WIDTH, HEIGHT);

		glfwSetWindowSizeCallback(window, [](GLFWwindow*, int width, int height) {glViewport(0, 0, width, height); });

		return window;
	}

	GLuint generateLinkedShaderProgram()
	{
		char infolog[256];
		int success = 0;
		static const char* vertextShaderSource =
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
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!vertexShader || !fragmentShader) return 0;

		glShaderSource(vertexShader, 1, &vertextShaderSource, nullptr);
		glCompileShader(vertexShader);
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertexShader, 256, nullptr, infolog);
			std::cerr << "failed to compile vertex shader\n" << infolog << std::endl;
			return 0;
		}

		glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragmentShader, 256, nullptr, infolog);
			std::cerr << "failed to compile fragment shader\n" << infolog << std::endl;
			return 0;
		}

		GLuint linkedProgram = glCreateProgram();
		if (!linkedProgram) return 0;

		glAttachShader(linkedProgram, vertexShader);
		glAttachShader(linkedProgram, fragmentShader);
		glLinkProgram(linkedProgram);

		glGetProgramiv(linkedProgram, GL_LINK_STATUS, &success);
		if(!success){
			glGetProgramInfoLog(linkedProgram, 256, nullptr, infolog);
			std::cerr << "failed to link shader program\n" << infolog << std::endl;
			return 0;
		}
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return linkedProgram;
	}

	bool generateElement(GLuint & EAO, GLuint & VAO, GLuint & VBO)
	{
		static float vertices1[] = {
			0.5f,  0.5f, 0.0f,  // top right
			0.5f, -0.5f, 0.0f,  // bottom right
			-0.5f, -0.5f, 0.0f,  // bottom left
			-0.5f,  0.5f, 0.0f   // top left 
		};

		static unsigned int indices[] = {
			0, 1, 3,
			1, 2, 3
		};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EAO);

		if (!VAO || !VBO) return false;

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(float), static_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		return true;
	}

	void processInputs(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

}

int main() {
	return HTPractice4::main();
}