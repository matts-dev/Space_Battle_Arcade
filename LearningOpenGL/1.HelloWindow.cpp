#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

namespace HelloWindow {

	//callback to handle resizing
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);

	//input processing callback
	void processInput(GLFWwindow* window);

	const GLsizei width = 800;
	const GLsizei height = 600;

	int main_hellowindow() {
		//set up glfw
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		//create window
		GLFWwindow* window = glfwCreateWindow(width, height, "LearnOpenGL", nullptr, nullptr);
		if (window == nullptr) {
			std::cout << "failed to create window" << std::endl;
			glfwTerminate();
			return -1;
		}
		glfwMakeContextCurrent(window);

		//check GLAD is working
		if (!gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress)) {
			std::cout << "failed to initialze glad" << std::endl;
		}

		//set up the view port dimensions
		glViewport(0, 0, width, height);

		//set up call back for resizing window
		glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);

		//render loop
		while (!glfwWindowShouldClose(window)) {
			//poll input
			processInput(window);

			//render commands
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			//swap and draw
			glfwSwapBuffers(window);
			glfwPollEvents(); //update keyboard input, mouse movement, etc.
		}

		//free resources before closing
		glfwTerminate();
		return 0;
	}

	void framebuffer_size_callback(GLFWwindow * window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void processInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { //GLFW_RELEASE is the state where no key is pressed 
			glfwSetWindowShouldClose(window, true);
		}
	}
}