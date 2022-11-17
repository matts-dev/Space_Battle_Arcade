#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../../nu_utils.h"
#include "../../../Shader.h"
#include "../../../Libraries/stb_image.h"
#include "../../GettingStarted/Camera/CameraFPS.h"
#include "../../../InputTracker.h"

namespace
{
	CameraFPS camera(45.0f, -90.f, 0.f);

	float lastFrameTime = 0.f;
	float deltaTime = 0.0f;

	float pitch = 0.f;
	float yaw = -90.f;

	float FOV = 45.0f;

	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				layout (location = 2) in vec2 textureCoordinates;
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;
				out vec2 interpTextCoords;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = mat3(transpose(inverse(model))) * normal;

					interpTextCoords = textureCoordinates;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D diffuseMap;   
					sampler2D specularMap;   //light's color impact on object
					int shininess;
				};
				uniform Material material;			

				struct Light
				{
					vec3 position;
					
					//intensities are like strengths -- but they may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;

					float constant;
					float linear;
					float quadratic;
				};	
				uniform Light light;

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;

				uniform bool bEnableGammaCorrection = false;

				void main(){
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	//diffuse color is generally similar to ambient color

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 normal = normalize(fragNormal);														//interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 averageVec = normalize(toView + toLight);
					float specularAmount = pow(max(dot(normal, averageVec), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation

					fragmentColor = vec4(lightContribution, 1.0f);
				}
			)";
	const char* lamp_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* lamp_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 color;
				
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";


	const char* gammacube_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* gammacube_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 color;
				
				void main(){
					fragmentColor = vec4(color, 1.f);

					float gamma = 2.2;
					fragmentColor.rgb = pow(fragmentColor.rgb, vec3(1.0f/gamma));
				}
			)";

	float yRotation = 0.f;
	bool rotateLight = false;
	bool toggleAmbient = true;
	bool toggleDiffuse = true;
	bool toggleSpecular = true;
	float ambientStrength = 0.2f;
	float diffuseStrength = 0.5f;
	float specularStrength = 1.f;
	int shininess = 32;
	float floatValIncrement = 0.25f;
	bool enableGamma = false;

	void processInput(GLFWwindow* window)
	{
		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

		if (input.isKeyJustPressed(window, GLFW_KEY_T))
		{
			enableGamma = !enableGamma;
		}

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			yRotation += 0.1f;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			yRotation -= 0.1f;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_R))
		{
			rotateLight = !rotateLight;
		}

		//ambient 
		if (input.isKeyJustPressed(window, GLFW_KEY_M))
			toggleAmbient = !toggleAmbient;
		if (input.isKeyDown(window, GLFW_KEY_U))
		{
			ambientStrength = ambientStrength + floatValIncrement * deltaTime;
			if (ambientStrength > 1) ambientStrength = 1;
		}
		if (input.isKeyDown(window, GLFW_KEY_J))
		{
			ambientStrength = ambientStrength - floatValIncrement * deltaTime;
			if (ambientStrength < 0) ambientStrength = 0;
		}


		//diffuse 
		if (input.isKeyJustPressed(window, GLFW_KEY_COMMA))
			toggleDiffuse = !toggleDiffuse;
		if (input.isKeyDown(window, GLFW_KEY_I))
		{
			diffuseStrength = diffuseStrength + floatValIncrement * deltaTime;
			if (diffuseStrength > 1) diffuseStrength = 1;
		}
		if (input.isKeyDown(window, GLFW_KEY_K))
		{
			diffuseStrength = diffuseStrength - floatValIncrement * deltaTime;
			if (diffuseStrength < 0) diffuseStrength = 0;
		}

		//specular
		if (input.isKeyJustPressed(window, GLFW_KEY_PERIOD))
			toggleSpecular = !toggleSpecular;
		if (input.isKeyDown(window, GLFW_KEY_O))
		{
			specularStrength = specularStrength + floatValIncrement * deltaTime;
			if (specularStrength > 1) specularStrength = 1;
		}
		if (input.isKeyDown(window, GLFW_KEY_L))
		{
			specularStrength = specularStrength - floatValIncrement * deltaTime;
			if (specularStrength < 0) specularStrength = 0;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_P))
		{
			shininess *= 2;
			std::cout << "new shininess: " << shininess << std::endl;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_SEMICOLON))
		{
			shininess = (shininess / 2) == 0 ? 1 : shininess / 2;
			std::cout << "new shininess: " << shininess << std::endl;
		}

		camera.handleInput(window, deltaTime);
	}

	void true_main()
	{
		camera.setPosition(0.0f, 0.0f, 3.0f);
		int width = 800;
		int height = 600;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		float vertices[] = {
			// positions______    ___normals _______   texture coords
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
		};

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.

							  //GENERATE LAMP
		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//textures
		GLuint diffuseMap = textureLoader("Textures/woodfloor.png", GL_TEXTURE0);
		GLuint specularMap = textureLoader("Textures/white.png", GL_TEXTURE1);

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.diffuseMap", 0);
		shader.setUniform1i("material.specularMap", 1);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);
		Shader demoCubeShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);
		Shader gammaCubeShader(gammacube_vertex_shader_src, gammacube_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 0.1f, 2.0f);
		glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

		glm::vec3 cubePositions[] = {
			glm::vec3(0.0f,  0.0f,  -1.0f),
			glm::vec3(0.0f,  -1.0f, 0.0f),
			glm::vec3(-1.5f, 2.2f, -2.5f)
		};

		glm::vec3 cubeScales[] = {
			glm::vec3(1, 1, 1),
			glm::vec3(10, 1, 10),
			glm::vec3(1, 1, 1)
		};

		glDisable(GL_FRAMEBUFFER_SRGB);
		//glEnable(GL_FRAMEBUFFER_SRGB);
		while (!glfwWindowShouldClose(window))
		{
			//--------------------------------------------------------------------------------------------------------------------------
			shader.setUniform1i("bEnableGammaCorrection", enableGamma);
			//--------------------------------------------------------------------------------------------------------------------------

			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);

			//draw light
			//lightcolor.x = static_cast<float>(sin(glfwGetTime() * 2.0f) / 2 + 0.5f);
			//lightcolor.y = static_cast<float>(sin(glfwGetTime() * 0.7f) / 2 + 0.5f);
			//lightcolor.z = static_cast<float>(sin(glfwGetTime() * 1.3f) / 2 + 0.5f);
			glm::vec3 diffuseColor = lightcolor * diffuseStrength;
			glm::vec3 ambientColor = diffuseColor * ambientStrength; //this is the tutorial did, seems like we should use lightcolor instead of diffuseColor.
																	 //glm::vec3 ambientColor = lightcolor * ambientStrength; //this doesn't look as good, ambient is too bright when light gets dim.

			glm::mat4 model;
			model = glm::rotate(model, glm::radians(rotateLight ? 100 * currentTime : 0), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightStart);
			model = glm::scale(model, glm::vec3(0.2f));
			lightPos = glm::vec3(model * glm::vec4(0, 0, 0, 1));

			lampShader.use();
			lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("color", lightcolor.x, lightcolor.y, lightcolor.z);
			glBindVertexArray(lampVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			//draw objects
			//tweak parameters
			shader.use();
			shader.setUniform1i("material.shininess", shininess * 8);
			shader.setUniform3f("light.ambientIntensity", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("light.diffuseIntensity", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("light.specularIntensity", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform1f("light.constant", 1);
			shader.setUniform1f("light.linear", 0.10f);
			shader.setUniform1f("light.quadratic", 0.065f);

			shader.setUniform1i("enableAmbient", toggleAmbient);
			shader.setUniform1i("enableDiffuse", toggleDiffuse);
			shader.setUniform1i("enableSpecular", toggleSpecular);
			shader.setUniform3f("light.position", lightPos.x, lightPos.y, lightPos.z);

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				glm::mat4 model(1.f); //set model to identity matrix
				model = glm::translate(model, cubePositions[i]);
				model = glm::scale(model, cubeScales[i]);

				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			//draw non-gamma corrected
			glm::vec3 rowPos(-10, 0, 0);
			for (size_t i = 0; i < 11; ++i)
			{
				glm::mat4 model(1.f); //set model to identity matrix
				glm::vec3 cubePos = rowPos + glm::vec3(i, 0, 0);
				model = glm::translate(model, cubePos);
				demoCubeShader.use();
				demoCubeShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				demoCubeShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				demoCubeShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				demoCubeShader.setUniform3f("color", glm::vec3(i * 0.1f));
				glBindVertexArray(lampVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}


			//draw gamma corrected
			for (size_t i = 0; i < 11; ++i)
			{
				glm::mat4 model(1.f); //set model to identity matrix
				glm::vec3 cubePos = rowPos + glm::vec3(i, 2, 0);
				model = glm::translate(model, cubePos);
				gammaCubeShader.use();
				gammaCubeShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				gammaCubeShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				gammaCubeShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				gammaCubeShader.setUniform3f("color", glm::vec3(i * 0.1f));
				glBindVertexArray(lampVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);

		glDeleteVertexArrays(1, &lampVAO);
	}
}

/*
point light constants chart from Ogre3D engine

distance	constant	linear		quadratic
7			1.0			0.7			1.8
13			1.0			0.35		0.44
20			1.0			0.22		0.20
32			1.0			0.14		0.07
50			1.0			0.09		0.032
65			1.0			0.07		0.017
100			1.0			0.045		0.0075
160			1.0			0.027		0.0028
200			1.0			0.022		0.0019
325			1.0			0.014		0.0007
600			1.0			0.007		0.0002
3250		1.0			0.0014		0.000007

*/

//int main()
//{
//	true_main();
//}