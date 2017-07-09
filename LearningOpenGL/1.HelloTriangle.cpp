#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

namespace HelloTriangle {

	//callback to handle resizing
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);

	//input processing callback
	void processInput(GLFWwindow* window);

	const GLsizei width = 800;
	const GLsizei height = 600;

	const char* vertextShader =
		"#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
		"}\n\0";

	const char* fragmentShader =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    //always render an orange color\n"
		"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
		"}\n\0";

	int main() {
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
			return -1;
		}

		//set up the view port dimensions
		glViewport(0, 0, width, height);

		//set up call back for resizing window
		glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);

		//-------------------------- SHADERS --------------------------------//
		//COMPILE VERTEX SHADER
		GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShaderID, 1, &vertextShader, nullptr); //secnod arg is number of strings you can pass it, 4th arg is an array of those string lengths
		glCompileShader(vertexShaderID);

		//test compilation of vertex shader 
		GLint vertexCompileSuccess = 0;
		glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &vertexCompileSuccess);
		if (!vertexCompileSuccess) {
			char log[512];
			glGetShaderInfoLog(vertexShaderID, 512, NULL, log);
			std::cout << "vertex shader compile fail" << log << std::endl;
		}

		//COMPILE FRAGMENT SHADER
		GLuint fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShaderID, 1, &fragmentShader, nullptr);
		glCompileShader(fragShaderID);

		//check success of fragment shader compilation
		GLint fragShaderSuccess = 0;
		glGetShaderiv(fragShaderID, GL_COMPILE_STATUS, &fragShaderSuccess);
		if (!fragShaderSuccess) {
			char log[512];
			glGetShaderInfoLog(fragShaderID, 512, nullptr, log);
			std::cout << "frag shader failed to compile\n" << log << std::endl;
		}

		//LINK SHADERS INTO SHADER PROGRAM
		GLuint shaderProgramID = glCreateProgram();
		glAttachShader(shaderProgramID, vertexShaderID);
		glAttachShader(shaderProgramID, fragShaderID);
		glLinkProgram(shaderProgramID);

		//test linking of shader program
		GLint shaderLinkSuccess = 0;
		glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &shaderLinkSuccess);
		if (!shaderLinkSuccess) {
			char infoLog[512]; //not recycling logs for example readability
			glGetProgramInfoLog(shaderProgramID, 512, nullptr, infoLog);
			std::cout << "failed to link shaders in shader program\n" << infoLog << std::endl;
		}

		//shader program is ready to be used

		//delete shader objects since we've already linked them into the shader program
		glDeleteShader(vertexShaderID);
		glDeleteShader(fragShaderID);

		// ------------------------- TRIANGLE DATA -------------------------//
		float vertices[] = {
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f, 0.5f, 0.0f
		};

		//create a VAO (vertex array object) to store the vertex attributes's configuration state
		GLuint VAO = 0;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO); //any state changes will not be recorded into this VAO until another is bound; bind to this VAO to restore state before drawing triangle

		//create VBO (vertex buffer object)
		GLuint VBO = 0;	//will hold id of buffer object responsible for traingle
		glGenBuffers(1, &VBO);	//create 1 buffer, pass address of where id should be stored 

		//bind (connect) the type GL_ARRAY_BUFFER to the generated buffer at ID "VBO"
		glBindBuffer(GL_ARRAY_BUFFER, VBO); //operations on GL_ARRAY_BUFFER will now	 affect the VBO buffer object

		//buffer (load) the data into the buffer we created (GL_ARRAY_BUFFER is bound to VBO, which is how VBO gets data)
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		//INSTRUCT OPENGL HOW TO INTERPRET PASSED DATA
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0)); // note: needs VBO already bound,

		glEnableVertexAttribArray(0); //activates vertex attribute (shader variable) labeled with location 0 

		//unbind the vertex array so other configuration changes are not stored in this VBA
		glBindVertexArray(0);

		glUseProgram(shaderProgramID);//set the shader program to use (note: the tutorial places in this in render loop before draw calls, but since we only have 1 shader program I am putting here)
		// -----------------------------------------------------------------//



		// --------------------------- RECTANGLE DATA -----------------------//

		//recycle points uses indices in element buffers
		float rectVertices[] = {
			0.5f,  0.5f, 0.0f,  // top right
			0.5f, -0.5f, 0.0f,  // bottom right
			-0.5f, -0.5f, 0.0f,  // bottom left
			-0.5f,  0.5f, 0.0f   // top left 
		};

		GLuint rectIndices[] = {
			0, 1, 3,	//first triangle
			1, 2, 3		//second triangle
		};

		GLuint VAO_Rect = 0;
		glGenVertexArrays(1, &VAO_Rect);
		glBindVertexArray(VAO_Rect);	//build state for VAO_rect

		GLuint VBO_RectPnts = 0;
		glGenBuffers(1, &VBO_RectPnts);
		glBindBuffer(GL_ARRAY_BUFFER, VBO_RectPnts);
		glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

		//element array buffer object stores how to use index data
		GLuint EBO = 0;
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //this is set by default, no need to do this call; just contrasting two potential modes

		//render loop
		while (!glfwWindowShouldClose(window)) {
			//poll input
			processInput(window);

			//CLEAR SCREEN (render commands)
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			//SET SHADER FOR DRAWING
			glUseProgram(shaderProgramID);

			//DRAW TRIANGLE with configuration saved in vao
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			//DRAW RECTANGLE using configuration saved in VAO
			glBindVertexArray(VAO_Rect);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); //notice this isn't draw*arrays*, but is draw*elements*

			//swap and draw
			glfwSwapBuffers(window);
			glfwPollEvents(); //update keyboard input, mouse movement, etc.

		}

		//deallocate resources
		glDeleteVertexArrays(1, &VAO); //triangle's resources
		glDeleteBuffers(1, &VBO);

		glDeleteVertexArrays(1, &VAO_Rect); //rectangle's resources
		glDeleteBuffers(1, &VBO_RectPnts);
		glDeleteBuffers(1, &EBO);

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

//int main() {
//	HelloTriangle::main();
//}
