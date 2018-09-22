#include <glad/glad.h>
//#include <GLFW/glfw3.h>
//#include "../../../Libraries/stb_image.h"
//
//#include <glm.hpp>
//#include <gtc/matrix_transform.hpp>
//#include <gtc/type_ptr.hpp>
//
//#include <string>
//#include <fstream>
//#include <sstream>
//#include <iostream>
//
//#include <vector>
//
//#include <iostream>
//
//using namespace std;
//
//namespace
//{
//	const char* anti_aliasing_vs = R"(
//#version 330 core
//layout (location = 0) in vec3 aPos;
//
//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;
//
//void main()
//{
//    gl_Position = projection * view * model * vec4(aPos, 1.0);
//}
//			)";
//	const char* anti_aliasing_fs = R"(
//#version 330 core
//out vec4 FragColor;
//
//void main()
//{
//    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
//}
//			)";
//	const char* aa_post_vs = R"(
//#version 330 core
//layout (location = 0) in vec2 aPos;
//layout (location = 1) in vec2 aTexCoords;
//
//out vec2 TexCoords;
//
//void main()
//{
//    TexCoords = aTexCoords;
//    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
//}  
//			)";
//	const char* aa_post_fs = R"(
//#version 330 core
//out vec4 FragColor;
//
//in vec2 TexCoords;
//
//uniform sampler2D screenTexture;
//
//void main()
//{
//    vec3 col = texture(screenTexture, TexCoords).rgb;
//    float grayscale = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
//    FragColor = vec4(vec3(grayscale), 1.0);
//} 
//			)";
//
//
//	class Shader
//	{
//	public:
//		unsigned int ID;
//		// constructor generates the shader on the fly
//		// ------------------------------------------------------------------------
//		Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
//		{
//			// 1. retrieve the vertex/fragment source code from filePath
//			std::string vertexCode;
//			std::string fragmentCode;
//			std::string geometryCode;
//			std::ifstream vShaderFile;
//			std::ifstream fShaderFile;
//			std::ifstream gShaderFile;
//			// ensure ifstream objects can throw exceptions:
//			vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//			fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//			gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//			try
//			{
//				// open files
//				//vShaderFile.open(vertexPath);
//				//fShaderFile.open(fragmentPath);
//				//std::stringstream vShaderStream, fShaderStream;
//				// read file's buffer contents into streams
//				//vShaderStream << vShaderFile.rdbuf();
//				//fShaderStream << fShaderFile.rdbuf();
//				// close file handlers
//				//vShaderFile.close();
//				//fShaderFile.close();
//				// convert stream into string
//				//vertexCode = vShaderStream.str();
//				//fragmentCode = fShaderStream.str();
//				vertexCode = vertexPath;
//				fragmentCode = fragmentPath;
//				// if geometry shader path is present, also load a geometry shader
//				/*if (geometryPath != nullptr)
//				{
//					gShaderFile.open(geometryPath);
//					std::stringstream gShaderStream;
//					gShaderStream << gShaderFile.rdbuf();
//					gShaderFile.close();
//					geometryCode = gShaderStream.str();
//				}*/
//			}
//			catch (std::ifstream::failure e)
//			{
//				std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
//			}
//			const char* vShaderCode = vertexCode.c_str();
//			const char * fShaderCode = fragmentCode.c_str();
//			// 2. compile shaders
//			unsigned int vertex, fragment;
//			// vertex shader
//			vertex = glCreateShader(GL_VERTEX_SHADER);
//			glShaderSource(vertex, 1, &vShaderCode, NULL);
//			glCompileShader(vertex);
//			checkCompileErrors(vertex, "VERTEX");
//			// fragment Shader
//			fragment = glCreateShader(GL_FRAGMENT_SHADER);
//			glShaderSource(fragment, 1, &fShaderCode, NULL);
//			glCompileShader(fragment);
//			checkCompileErrors(fragment, "FRAGMENT");
//			// if geometry shader is given, compile geometry shader
//			unsigned int geometry;
//			if (geometryPath != nullptr)
//			{
//				const char * gShaderCode = geometryCode.c_str();
//				geometry = glCreateShader(GL_GEOMETRY_SHADER);
//				glShaderSource(geometry, 1, &gShaderCode, NULL);
//				glCompileShader(geometry);
//				checkCompileErrors(geometry, "GEOMETRY");
//			}
//			// shader Program
//			ID = glCreateProgram();
//			glAttachShader(ID, vertex);
//			glAttachShader(ID, fragment);
//			if (geometryPath != nullptr)
//				glAttachShader(ID, geometry);
//			glLinkProgram(ID);
//			checkCompileErrors(ID, "PROGRAM");
//			// delete the shaders as they're linked into our program now and no longer necessery
//			glDeleteShader(vertex);
//			glDeleteShader(fragment);
//			if (geometryPath != nullptr)
//				glDeleteShader(geometry);
//
//		}
//		// activate the shader
//		// ------------------------------------------------------------------------
//		void use()
//		{
//			glUseProgram(ID);
//		}
//		// utility uniform functions
//		// ------------------------------------------------------------------------
//		void setBool(const std::string &name, bool value) const
//		{
//			glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
//		}
//		// ------------------------------------------------------------------------
//		void setInt(const std::string &name, int value) const
//		{
//			glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
//		}
//		// ------------------------------------------------------------------------
//		void setFloat(const std::string &name, float value) const
//		{
//			glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
//		}
//		// ------------------------------------------------------------------------
//		void setVec2(const std::string &name, const glm::vec2 &value) const
//		{
//			glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
//		}
//		void setVec2(const std::string &name, float x, float y) const
//		{
//			glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
//		}
//		// ------------------------------------------------------------------------
//		void setVec3(const std::string &name, const glm::vec3 &value) const
//		{
//			glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
//		}
//		void setVec3(const std::string &name, float x, float y, float z) const
//		{
//			glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
//		}
//		// ------------------------------------------------------------------------
//		void setVec4(const std::string &name, const glm::vec4 &value) const
//		{
//			glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
//		}
//		void setVec4(const std::string &name, float x, float y, float z, float w)
//		{
//			glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
//		}
//		// ------------------------------------------------------------------------
//		void setMat2(const std::string &name, const glm::mat2 &mat) const
//		{
//			glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
//		}
//		// ------------------------------------------------------------------------
//		void setMat3(const std::string &name, const glm::mat3 &mat) const
//		{
//			glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
//		}
//		// ------------------------------------------------------------------------
//		void setMat4(const std::string &name, const glm::mat4 &mat) const
//		{
//			glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
//		}
//
//	private:
//		// utility function for checking shader compilation/linking errors.
//		// ------------------------------------------------------------------------
//		void checkCompileErrors(GLuint shader, std::string type)
//		{
//			GLint success;
//			GLchar infoLog[1024];
//			if (type != "PROGRAM")
//			{
//				glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//				if (!success)
//				{
//					glGetShaderInfoLog(shader, 1024, NULL, infoLog);
//					std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
//				}
//			}
//			else
//			{
//				glGetProgramiv(shader, GL_LINK_STATUS, &success);
//				if (!success)
//				{
//					glGetProgramInfoLog(shader, 1024, NULL, infoLog);
//					std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
//				}
//			}
//		}
//	};
//
//	// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
//	enum Camera_Movement
//	{
//		FORWARD,
//		BACKWARD,
//		LEFT,
//		RIGHT
//	};
//
//	// Default camera values
//	const float YAW = -90.0f;
//	const float PITCH = 0.0f;
//	const float SPEED = 2.5f;
//	const float SENSITIVITY = 0.1f;
//	const float ZOOM = 45.0f;
//
//
//	// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
//	class Camera
//	{
//	public:
//		// Camera Attributes
//		glm::vec3 Position;
//		glm::vec3 Front;
//		glm::vec3 Up;
//		glm::vec3 Right;
//		glm::vec3 WorldUp;
//		// Euler Angles
//		float Yaw;
//		float Pitch;
//		// Camera options
//		float MovementSpeed;
//		float MouseSensitivity;
//		float Zoom;
//
//		// Constructor with vectors
//		Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
//		{
//			Position = position;
//			WorldUp = up;
//			Yaw = yaw;
//			Pitch = pitch;
//			updateCameraVectors();
//		}
//		// Constructor with scalar values
//		Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
//		{
//			Position = glm::vec3(posX, posY, posZ);
//			WorldUp = glm::vec3(upX, upY, upZ);
//			Yaw = yaw;
//			Pitch = pitch;
//			updateCameraVectors();
//		}
//
//		// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
//		glm::mat4 GetViewMatrix()
//		{
//			return glm::lookAt(Position, Position + Front, Up);
//		}
//
//		// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
//		void ProcessKeyboard(Camera_Movement direction, float deltaTime)
//		{
//			float velocity = MovementSpeed * deltaTime;
//			if (direction == FORWARD)
//				Position += Front * velocity;
//			if (direction == BACKWARD)
//				Position -= Front * velocity;
//			if (direction == LEFT)
//				Position -= Right * velocity;
//			if (direction == RIGHT)
//				Position += Right * velocity;
//		}
//
//		// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
//		void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
//		{
//			xoffset *= MouseSensitivity;
//			yoffset *= MouseSensitivity;
//
//			Yaw += xoffset;
//			Pitch += yoffset;
//
//			// Make sure that when pitch is out of bounds, screen doesn't get flipped
//			if (constrainPitch)
//			{
//				if (Pitch > 89.0f)
//					Pitch = 89.0f;
//				if (Pitch < -89.0f)
//					Pitch = -89.0f;
//			}
//
//			// Update Front, Right and Up Vectors using the updated Euler angles
//			updateCameraVectors();
//		}
//
//		// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
//		void ProcessMouseScroll(float yoffset)
//		{
//			if (Zoom >= 1.0f && Zoom <= 45.0f)
//				Zoom -= yoffset;
//			if (Zoom <= 1.0f)
//				Zoom = 1.0f;
//			if (Zoom >= 45.0f)
//				Zoom = 45.0f;
//		}
//
//	private:
//		// Calculates the front vector from the Camera's (updated) Euler Angles
//		void updateCameraVectors()
//		{
//			// Calculate the new Front vector
//			glm::vec3 front;
//			front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
//			front.y = sin(glm::radians(Pitch));
//			front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
//			Front = glm::normalize(front);
//			// Also re-calculate the Right and Up vector
//			Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
//			Up = glm::normalize(glm::cross(Right, Front));
//		}
//	};
//
//	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//	//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//	//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//	void processInput(GLFWwindow *window);
//
//	// settings
//	const unsigned int SCR_WIDTH = 1280;
//	const unsigned int SCR_HEIGHT = 720;
//
//	// camera
//	Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
//	float lastX = (float)SCR_WIDTH / 2.0;
//	float lastY = (float)SCR_HEIGHT / 2.0;
//	bool firstMouse = true;
//
//	// timing
//	float deltaTime = 0.0f;
//	float lastFrame = 0.0f;
//
//	int true_main()
//	{
//		// glfw: initialize and configure
//		// ------------------------------
//		glfwInit();
//		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//#ifdef __APPLE__
//		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
//#endif
//
//		GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
//		if (window == NULL)
//		{
//			std::cout << "Failed to create GLFW window" << std::endl;
//			glfwTerminate();
//			return -1;
//		}
//		glfwMakeContextCurrent(window);
//		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
//		//glfwSetCursorPosCallback(window, mouse_callback);
//		//glfwSetScrollCallback(window, scroll_callback);
//
//		// tell GLFW to capture our mouse
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//
//		// glad: load all OpenGL function pointers
//		// ---------------------------------------
//		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//		{
//			std::cout << "Failed to initialize GLAD" << std::endl;
//			return -1;
//		}
//
//		// configure global opengl state
//		// -----------------------------
//		glEnable(GL_DEPTH_TEST);
//
//		// build and compile shaders
//		// -------------------------
//		Shader shader(anti_aliasing_vs, anti_aliasing_fs);
//		Shader screenShader(aa_post_vs, aa_post_fs);
//
//		// set up vertex data (and buffer(s)) and configure vertex attributes
//		// ------------------------------------------------------------------
//		float cubeVertices[] = {
//			// positions       
//			-0.5f, -0.5f, -0.5f,
//			0.5f, -0.5f, -0.5f,
//			0.5f,  0.5f, -0.5f,
//			0.5f,  0.5f, -0.5f,
//			-0.5f,  0.5f, -0.5f,
//			-0.5f, -0.5f, -0.5f,
//
//			-0.5f, -0.5f,  0.5f,
//			0.5f, -0.5f,  0.5f,
//			0.5f,  0.5f,  0.5f,
//			0.5f,  0.5f,  0.5f,
//			-0.5f,  0.5f,  0.5f,
//			-0.5f, -0.5f,  0.5f,
//
//			-0.5f,  0.5f,  0.5f,
//			-0.5f,  0.5f, -0.5f,
//			-0.5f, -0.5f, -0.5f,
//			-0.5f, -0.5f, -0.5f,
//			-0.5f, -0.5f,  0.5f,
//			-0.5f,  0.5f,  0.5f,
//
//			0.5f,  0.5f,  0.5f,
//			0.5f,  0.5f, -0.5f,
//			0.5f, -0.5f, -0.5f,
//			0.5f, -0.5f, -0.5f,
//			0.5f, -0.5f,  0.5f,
//			0.5f,  0.5f,  0.5f,
//
//			-0.5f, -0.5f, -0.5f,
//			0.5f, -0.5f, -0.5f,
//			0.5f, -0.5f,  0.5f,
//			0.5f, -0.5f,  0.5f,
//			-0.5f, -0.5f,  0.5f,
//			-0.5f, -0.5f, -0.5f,
//
//			-0.5f,  0.5f, -0.5f,
//			0.5f,  0.5f, -0.5f,
//			0.5f,  0.5f,  0.5f,
//			0.5f,  0.5f,  0.5f,
//			-0.5f,  0.5f,  0.5f,
//			-0.5f,  0.5f, -0.5f
//		};
//		float quadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
//								   // positions   // texCoords
//			-1.0f,  1.0f,  0.0f, 1.0f,
//			-1.0f, -1.0f,  0.0f, 0.0f,
//			1.0f, -1.0f,  1.0f, 0.0f,
//
//			-1.0f,  1.0f,  0.0f, 1.0f,
//			1.0f, -1.0f,  1.0f, 0.0f,
//			1.0f,  1.0f,  1.0f, 1.0f
//		};
//		// setup cube VAO
//		unsigned int cubeVAO, cubeVBO;
//		glGenVertexArrays(1, &cubeVAO);
//		glGenBuffers(1, &cubeVBO);
//		glBindVertexArray(cubeVAO);
//		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
//		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
//		glEnableVertexAttribArray(0);
//		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//		// setup screen VAO
//		unsigned int quadVAO, quadVBO;
//		glGenVertexArrays(1, &quadVAO);
//		glGenBuffers(1, &quadVBO);
//		glBindVertexArray(quadVAO);
//		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
//		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
//		glEnableVertexAttribArray(0);
//		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
//		glEnableVertexAttribArray(1);
//		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
//
//
//		// configure MSAA framebuffer
//		// --------------------------
//		// -------------------------------------- MSAA FRAME BUFFER SETUP-------------------------------------------------------------
//		unsigned int framebuffer;
//		glGenFramebuffers(1, &framebuffer);
//		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//
//		// create a multisampled color attachment texture
//		unsigned int textureColorBufferMultiSampled;
//		glGenTextures(1, &textureColorBufferMultiSampled);
//		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
//		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
//		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
//
//		// create a (also multisampled) renderbuffer object for depth and stencil attachments
//		unsigned int rbo;
//		glGenRenderbuffers(1, &rbo);
//		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
//		glBindRenderbuffer(GL_RENDERBUFFER, 0);
//		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
//
//		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//		// -------------------------------------- SAMPLE RESOLVING FB-------------------------------------------------------------
//
//		// configure second post-processing framebuffer
//		unsigned int intermediateFBO;
//		glGenFramebuffers(1, &intermediateFBO);
//		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
//		// create a color attachment texture
//		unsigned int screenTexture;
//		glGenTextures(1, &screenTexture);
//		glBindTexture(GL_TEXTURE_2D, screenTexture);
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);	// we only need a color buffer
//
//		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//			cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//		// -------------------------------------- END RESOLVING FB-------------------------------------------------------------
//
//		// shader configuration
//		// --------------------
//		shader.use();
//		screenShader.setInt("screenTexture", 0);
//
//		// render loop
//		// -----------
//		while (!glfwWindowShouldClose(window))
//		{
//			// per-frame time logic
//			// --------------------
//			float currentFrame = glfwGetTime();
//			deltaTime = currentFrame - lastFrame;
//			lastFrame = currentFrame;
//
//			// input
//			// -----
//			//processInput(window);
//
//			// render
//			// ------
//			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//			// 1. draw scene as normal in multisampled buffers
//			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//			glEnable(GL_DEPTH_TEST);
//
//			// set transformation matrices		
//			shader.use();
//			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
//			shader.setMat4("projection", projection);
//			shader.setMat4("view", camera.GetViewMatrix());
//			shader.setMat4("model", glm::mat4());
//
//			glBindVertexArray(cubeVAO);
//			glDrawArrays(GL_TRIANGLES, 0, 36);
//
//			// 2. now blit multisampled buffer(s) to normal colorbuffer of intermediate FBO. Image is stored in screenTexture
//			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
//			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
//			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
//
//			// 3. now render quad with scene's visuals as its texture image
//			glBindFramebuffer(GL_FRAMEBUFFER, 0);
//			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//			glClear(GL_COLOR_BUFFER_BIT);
//			glDisable(GL_DEPTH_TEST);
//
//			// draw Screen quad
//			screenShader.use();
//			glBindVertexArray(quadVAO);
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, screenTexture); // use the now resolved color attachment as the quad's texture
//			glDrawArrays(GL_TRIANGLES, 0, 6);
//
//			// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
//			// -------------------------------------------------------------------------------
//			glfwSwapBuffers(window);
//			glfwPollEvents();
//		}
//
//		glfwTerminate();
//		return 0;
//	}
//
//	// glfw: whenever the window size changed (by OS or user resize) this callback function executes
//	// ---------------------------------------------------------------------------------------------
//	void framebuffer_size_callback(GLFWwindow* window, int width, int height)
//	{
//		// make sure the viewport matches the new window dimensions; note that width and 
//		// height will be significantly larger than specified on retina displays.
//		glViewport(0, 0, width, height);
//	}
//}
//
////int main()
////{
////	true_main();
////
////}