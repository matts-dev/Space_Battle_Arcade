#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include<string>
#include<iostream>
#include<fstream>
#include<sstream>

#include"Utilities.h"
#include"Shader.h"

namespace shaders {
	int main() {
		GLFWwindow* window = Utils::initOpenGl(800, 400);
		if (!window) { return -1; }

		//Check the maximum number of vertex attributes available on this system 
		int numVertAttribs = 0;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numVertAttribs);
		std::cout << "The number of vertex shader attributes on the system is: " << numVertAttribs << std::endl;
		
		//Shader basicShader("BasicVertexShader.glsl", "BasicFragShader.glsl");
		//Shader basicShader("3a.VertWithSingleOut.glsl", "3a.FragWithSingleIn.glsl");
		//Shader basicShader("3a.VertWithSingleOut.glsl", "3b.FragShaderWithUniform.glsl");
		Shader basicShader("3a.VertWithSingleOut.glsl", "3b.FragShaderWithUniform.glsl");
		if (basicShader.createFailed()) { glfwTerminate(); return -1; }

		Shader twoAttribShader("3c.2AtribVertShader.glsl", "3c.2AtribFragShader.glsl");
		if (twoAttribShader.createFailed()) { glfwTerminate(); return -1; }

		Shader flippedShaderChallenge1("3.DFlippedVertexShader_Challenge1.glsl", "3c.2AtribFragShader.glsl");
		if (flippedShaderChallenge1.createFailed()) { glfwTerminate(); return -1; }

		Shader challenge2ShaderOffset("3.EOffsetUniformFragShader.glsl", "BasicFragShader.glsl");
		if (challenge2ShaderOffset.createFailed()) { glfwTerminate(); return -1; }

		Shader challenge3ShaderVertexColor("3.FVertexOutToFragForColor.glsl", "3a.FragWithSingleIn.glsl");
		if (challenge3ShaderVertexColor.createFailed()) { glfwTerminate(); return -1; }

		GLuint VAO, VBO, EAO;
		//Utils::createSingleElementTriangle(EAO, VAO, VBO); // enable for shader: "FragShaderWithUniform" color fade code with element
		Utils::createTriangleWithTwoVertexAttributes(VAO, VBO);

		while (!glfwWindowShouldClose(window)) {
			glClearColor(0.f, 0.f, 0.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);


			//"3b. FragShaderWithUniform" color fade code with element
			//basicShader.use();
			//float timeValue = static_cast<float>(glfwGetTime());
			//float redValue = (std::sin(timeValue) / 2.0f) + 0.5f;
			//basicShader.setFloatUniform("globalColor", redValue, 0.0f, 0.0f, 1.0f);
			//glBindVertexArray(VAO);
			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO);
			//glBindBuffer(GL_ARRAY_BUFFER, VBO);
			//glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

			//code for rainbow triangle! 
			//flippedShaderChallenge1.use();
			//glBindVertexArray(VAO);
			//glDrawArrays(GL_TRIANGLES, 0, 3);

			//challenge 2 code
			//challenge2ShaderOffset.use();
			//challenge2ShaderOffset.setFloatUniform("offset", 0.5f, 0.f, 0.f, 0.f);
			//glBindVertexArray(VAO);
			//glDrawArrays(GL_TRIANGLES, 0, 3);

			//challenge 3 code			
			challenge3ShaderVertexColor.use();
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);
		}

		glfwTerminate();
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EAO);
		return 0;
	}

}


//int main() {
//	//std::ifstream inFileStream;
//	//inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
//	//std::stringstream strStream;
//
//	//std::string vertexShaderSrc, fragShaderSrc;
//	//if (!Utils::convertFileToString("BasicVertexShader.glsl", vertexShaderSrc)) { std::cerr << "failed to load vertex shader src" << std::endl; return -1; }
//	//if (!Utils::convertFileToString("BasicFragShader.glsl", fragShaderSrc)) { std::cerr << "failed to load frag shader src" << std::endl; return -1; }
//
//	//std::cout << vertexShaderSrc << std::endl;
//	//std::cout << fragShaderSrc << std::endl;
//
//	//int x;
//	//std::cin >> x;
//
//	return shaders::main();
//}
