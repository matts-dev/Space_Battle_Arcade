#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/InputTracker.h"
#include <gtx/wrap.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace
{
	// std alignment rules: 
	// The smallest addressable in std140 is 4bytes, we will label that as N; so n = 4bytes
	// Scalars require 1n of space, so 4 bytes
	// vec_ require 2n or 4n; vec3 requires 4n, which is 16bytes
	// matrices 4 vec4s, for 16n, which is 64 bytes; each is a vec4 though so it appears it can be aligned on multiples of 16, not multiples of 64
	// struct previous rules, with padding to make it a multiple of vec4

	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				

				layout (std140) uniform BlockData
				{					//base alignment		//aligned offset
					float v0Color;	//4						0
					vec3 v1Color;	//16					16
					mat4 v2Color00; //16					32 (col1) //notice this is aligned on a multiple that isn't of 64
									//16					48 (col2)
									//16					64 (col3)
									//16					80 (col4)
					float v3Color[3];//16					96 [0]
									//16					112[1]
									//16					128[2]
					bool v4Color;   //4						144
					int v5ColorR;   //4					    148
									//0						152
				};
				
				out vec3 color;			

				void main(){
					gl_Position = vec4(position, 1);
					
					if(gl_VertexID == 0){
						color = vec3(0, 0, v0Color);
					} 
					else if(gl_VertexID == 1){
						color = v1Color;
					}
					else if(gl_VertexID == 2){
						color = vec3(v2Color00[0]);
					}
					else if(gl_VertexID == 3){
						color = vec3(v3Color[0], v3Color[1], v3Color[2]);
					}
					else if(gl_VertexID == 4){
						color = vec3(0, v4Color,0);
					}
					else if(gl_VertexID == 5){
						color = vec3(v5ColorR, 0, 0);
					}
					else {
						color = vec3(1, 1, 1);
					}
					
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec3 color;

				void main(){
					fragmentColor = vec4(color, 1.0f);
				}
			)";


	float clamp(float val, float min, float max)
	{
		val = val < min ? min : val;
		val = val > max ? max : val;
		return val;
	}

	void processInput(GLFWwindow* window)
	{
		static InputTracker input;
		input.updateState(window);

		
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
	}

	void true_main()
	{
		GLFWwindow* window = init_window(800, 600);

		glViewport(0, 0, 800, 600);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });

		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &vertex_shader_src, nullptr);
		glCompileShader(vertShader);
		verifyShaderCompiled("vertex shader", vertShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &frag_shader_src, nullptr);
		glCompileShader(fragShader);
		verifyShaderCompiled("fragment shader", fragShader);

		GLuint shaderProg = glCreateProgram();
		glAttachShader(shaderProg, vertShader);
		glAttachShader(shaderProg, fragShader);
		glLinkProgram(shaderProg);
		verifyShaderLink(shaderProg);

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);

		float tri1[] = {
			0.5f, 0.5f, 0.0f,	 
			0.5f, -0.5f, 0.0f,   
			-0.5f, 0.5f, 0.0f,   
		};

		float tri2[] = {
			0.5f, -0.5f, 0.0f,   
			-0.5f, -0.5f, 0.0f,  
			-0.5f, 0.5f, 0.0f
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tri1) + sizeof(tri2), nullptr, GL_DYNAMIC_DRAW); //null just allocates the memory without actually filling anything into them memory

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.

		//-------------------------------------------------- advanced data ---------------------------------------------------------------------

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri1), tri1);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(tri1), sizeof(tri2), tri2);

		//--------------------------------------------------- uniform buffer --------------------------------------------------------------------
		//CREATE BUFFER
		GLuint uboBlockBuffer;
		glGenBuffers(1, &uboBlockBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, uboBlockBuffer);
		//glBufferData(GL_UNIFORM_BUFFER, 152, nullptr, GL_STATIC_DRAW); 
		glBufferData(GL_UNIFORM_BUFFER, 160, nullptr, GL_STATIC_DRAW); //makes renderdoc happy; multiple of 4n(16bytes)
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		//BIND SHADER'S BLOCK TO BINDING_POINT
		//this is 'like' querying a uniforms location, but uses the language of 'index'
		GLuint blockBindingPoint = 1;
		GLuint blockUniformLocIdx = glGetUniformBlockIndex(shaderProg, "BlockData");
		glUniformBlockBinding(shaderProg, blockUniformLocIdx, blockBindingPoint);    //notice parameter order here isn't consistent with the following, the block binding comes last

		//BIND BUFFER TO BINDING POINT
		glBindBuffer(GL_UNIFORM_BUFFER, uboBlockBuffer); //set current uniform buffer target
		glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingPoint, uboBlockBuffer);  //configure entire buffer to binding point
		//glBindBufferRange(GL_UNIFORM_BUFFER, blockBindingPoint, uboBlockBuffer, 0, 152); //allows customizing a binding point to a range of the created buffer; we're just doing the full buffer range

		//UPDATE BUFFER CONTENTS, THUS UPDATING UNIFORM 
		/*
		layout(std140) uniform BlockData
		{					//base alignment		//aligned offset
			float v0Color;	//4						0
			vec3 v1Color;	//16					16
			mat4 v2Color00; //16					32 (col1) //notice this is aligned on a multiple that isn't of 64
							//16					48 (col2)
							//16					64 (col3)
							//16					80 (col4)
			float v3Color[3];//16					96 [0]
							 //16					112[1]
							 //16					128[2]
			bool v4Color;   //4						144
			int v5ColorR;   //4					    148
							//0						152
		};
		*/
		glBindBuffer(GL_UNIFORM_BUFFER, uboBlockBuffer);

		float v0Color = 1.0f;
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float)*4, &v0Color);

		glm::vec3 v1Color(0.0f, 0.25f, 0.5f);
		glBufferSubData(GL_UNIFORM_BUFFER, 16, sizeof(float) * 4, &v1Color);

		glm::mat4 v2Color;
		v2Color[0][0] = 0.5f;
		v2Color[0][1] = 0.5f;
		v2Color[0][2] = 0.0f;
		glBufferSubData(GL_UNIFORM_BUFFER, 32, /*64*/ sizeof(glm::mat4), glm::value_ptr(v2Color));

		float v3Color[] = { 0.5f, 0.0f, 0.5f }; //pinkish color
		glBufferSubData(GL_UNIFORM_BUFFER, 96, /*12*/ sizeof(v3Color), &v3Color);

		int v4Color = 1; //bool at 144
		glBufferSubData(GL_UNIFORM_BUFFER, 144, 4, &v4Color);

		int v5Color = 0; //int at 148, should be black
		glBufferSubData(GL_UNIFORM_BUFFER, 148, 4, &v5Color);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		//--------------------------------------------------------------------------------------------------------------------------


		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe mode
		while (!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0, 0, 0, 1.0f);
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