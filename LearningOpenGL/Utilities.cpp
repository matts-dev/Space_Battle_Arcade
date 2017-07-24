#include"Utilities.h"
#include<fstream>
#include<sstream>

namespace Utils {

	bool convertFileToString(const std::string filePath, std::string& strRef)
	{
		std::ifstream inFileStream;
		inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
		std::stringstream strStream;

		//instead of using exceptions, we can check inFileStream.is_open(); but it is good to learn some new stuff! (though I've heard c++ community doesn't like exceptions right now)
		try {
			inFileStream.open(filePath);
			strStream << inFileStream.rdbuf();
			strRef = strStream.str();

			//clean up
			inFileStream.close();
			strStream.str(std::string()); //clear string stream
			strStream.clear(); //clear error states
		}
		catch (std::ifstream::failure e) {
			std::cerr << "failed to open file\n" << e.what() << std::endl;
			return false;
		}

		return true;
	}

	GLFWwindow* initOpenGl(const int width, const int height)
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Window", nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			return nullptr;
		}

		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
			std::cerr << "Failed to set up GLAD" << std::endl;
			glfwTerminate();
			return nullptr;
		}

		glViewport(0, 0, width, height);

		//instead of passing a callback, just do a simple lambda 
		glfwSetWindowSizeCallback(window, [](GLFWwindow*window, int width, int height) {glViewport(0, 0, width, height); });

		return window;
	}

	void setWindowCloseOnEscape(GLFWwindow * window)
	{
		if (window) {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
		}
	}

	bool createSingleElementTriangle(GLuint & EAO, GLuint & VAO, GLuint & VBO)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EAO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Utils::basicTriangleVertices), basicTriangleVertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Utils::indices), indices, GL_STATIC_DRAW);

		//assuming using the basic shader that has attribute at layout location 0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
		glEnableVertexAttribArray(0);
		
		//unbind VAO so further state changes are not saved
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		//make sure non of the buffers were set to 0
		return (VAO && VBO && EAO);
	}

}
