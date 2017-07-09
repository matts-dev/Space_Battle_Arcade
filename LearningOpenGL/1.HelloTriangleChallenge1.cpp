//1. Try to draw 2 triangles next to each other using glDrawArrays by adding more vertices to your data :


#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

namespace HT_Challenge1 {

	const int WIDTH = 800;
	const int HEIGHT = 600;

	//Forward declarations
	void processInput(GLFWwindow* window);
	void framebuffer_resizecallback(GLFWwindow*, int height, int width);
	GLFWwindow* SetUpOpenGL();
	GLuint CreateBasicShader();
	GLuint createTriangleRetVAO(float* vertices, unsigned int verticesSize, GLuint& VAO, GLuint& VBO);

	int main() {
		GLFWwindow* window = SetUpOpenGL();
		if (!window) return -1;

		//triangle 1
		float vertices1[] = {
			0.f, -0.75f, 0.0f,
			0.f, 0.75f, 0.0f,
			-0.5f,  -0.0f, 0.0f
		};

		//triangle 1
		float vertices2[] = {
			0.f, -0.75f, 0.0f,
			0.f, 0.75f, 0.0f,
			0.5f,  -0.0f, 0.0f
		};

		GLuint linkedShaderProgram = CreateBasicShader();
		if (!linkedShaderProgram) {	return -1;	}

		GLuint VAO1, VBO1;
		createTriangleRetVAO(vertices1, sizeof(vertices1), VAO1, VBO1);

		//glGenVertexArrays(1, &VAO1);
		//glGenBuffers(1, &VBO1);
		//glBindVertexArray(VAO1); //save configuration in this vertex array object
		//glBindBuffer(GL_ARRAY_BUFFER, VBO1);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
		//glEnableVertexAttribArray(0);

		//glBindVertexArray(0); //stop saving configurations under this VAO

		GLuint VAO2, VBO2;
		createTriangleRetVAO(vertices2, sizeof(vertices2), VAO2, VBO2);

		while (!glfwWindowShouldClose(window)) {
			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(linkedShaderProgram);

			glBindVertexArray(VAO1);
			glDrawArrays(GL_TRIANGLES, 0, 3);	

			glBindVertexArray(VAO2);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		//clear up (next time I'll store both VBOs and VBAs in an array)
		glDeleteBuffers(1, &VBO1);
		glDeleteBuffers(1, &VBO2);
		glDeleteVertexArrays(1, &VAO1);
		glDeleteVertexArrays(1, &VAO2);

		glfwTerminate();
		return 0;
	}

	// --------------------------------------------------------------------------------------------------------------- Defs

	void processInput(GLFWwindow* window) {
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
	}
	void framebuffer_resizecallback(GLFWwindow *, int height, int width) {
		glViewport(0, 0, height, width);
	}

	GLFWwindow * SetUpOpenGL()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_COMPAT_PROFILE, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "HT_Challenge1", nullptr, nullptr);
		if (!window) {
			std::cerr << "failed to create window" << std::endl;
			glfwTerminate(); return nullptr;
		}
		glfwMakeContextCurrent(window);

		//static cast gave issues, I think it was due to the return type
		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
			std::cerr << "GLAD init failed" << std::endl;
			glfwTerminate(); return nullptr;
		}

		glViewport(0, 0, WIDTH, HEIGHT);
		glfwSetWindowSizeCallback(window, &framebuffer_resizecallback);

		return window;
	}

	GLuint CreateBasicShader()
	{
		const char* vertexShaderSource =
			"#version 330 core\n"
			"layout (location = 0) in vec3 aPos;\n"
			"void main()\n"
			"{\n"
			"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
			"}\n\0";

		const char* fragmentShaderSource =
			"#version 330 core\n"
			"out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"    //always render an orange color\n"
			"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
			"}\n\0";

		
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragShader);

		char infolog[256];
		GLint success = 0;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); 
		if (!success) {
			glGetShaderInfoLog(vertexShader, 256, 0, infolog);
			std::cerr << "failed to compile vertex shader\n" << infolog << std::endl;
			return 0;
		}

		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(fragShader, 256, 0, infolog);
			std::cerr << "failed to compile frag shader\n" << infolog << std::endl;
			return 0;
		}

		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragShader);
		glLinkProgram(shaderProgram);

		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shaderProgram, 256, 0, infolog);
			std::cerr << "failed to link program" << infolog << std::endl;
			return 0;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragShader);

		return shaderProgram;
	}

	GLuint createTriangleRetVAO(float* vertices, unsigned int verticesSize, GLuint& VAO, GLuint& VBO)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO); //start storing state in VAO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);	//move vertice data onto graphics card memory pt1: make active array GL_ARRAY_BUFFER
		glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);		//move vertice data onto graphics card memory pt2: buffer data into VBO bound to GL_ARRAY_BUFFER
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0)); //configure attribut pointer
		glEnableVertexAttribArray(0); 		//enable the 0th attribute in the shader

		//unbind VAO to prevent adding/overwriting configured state
		glBindVertexArray(0);

		return VAO;
	}
}

int main() {
	HT_Challenge1::main();
}