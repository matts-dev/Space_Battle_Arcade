#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"
#include "ReferenceCode/OpenGL/Utilities/SphereMesh.h"
#include "ReferenceCode/OpenGL/InputTracker.h"
#include "ReferenceCode/OpenGL/Utilities/SphereMeshTextured.h"

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
				layout (location = 2) in vec2 inTexCoords;
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;
				out vec2 texCoords;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = mat3(transpose(inverse(model))) * normal;

					texCoords = inTexCoords;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				uniform vec3 objectColor;
				uniform vec3 lightPosition;
				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 texCoords;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform float ambientStrength = 0.1f; 

				struct Material {
					sampler2D albedo;		//diffuse
					sampler2D normal;
					sampler2D metalic;
					sampler2D roughness;
					sampler2D AO;			//ambient occlusion
				};
				uniform Material material;

				void main(){
					vec3 ambientLight = ambientStrength * lightColor;					

					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 normal = normalize(fragNormal); //interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * lightColor;

					float specularStrength = 0.5f;
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 toReflection = reflect(-toView, normal); //reflect expects vector from light position (tutorial didn't normalize this vector)
					float specularAmount = pow(max(dot(toReflection, toLight), 0), 32);
					vec3 specularLight = specularStrength * lightColor * specularAmount;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * texture(material.albedo, texCoords).rgb;
					
					fragmentColor = vec4(lightContribution, 1.0f);

					//fragmentColor = vec4(texture(material.normal, texCoords).rgb, 1.0f);
					//fragmentColor = vec4(texture(material.roughness, texCoords).rgb, 1.0f);
					//fragmentColor = vec4(texture(material.metalic, texCoords).rgb, 1.0f);
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

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";


	bool rotateLight = true;
	bool bUseWireFrame = false;
	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press R to toggle light rotation" << std::endl
				<< " " << std::endl
				<< " " << std::endl
				<< std::endl;
			return 0;
		}();
		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);


		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_R))
		{
			rotateLight = !rotateLight;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			bUseWireFrame = !bUseWireFrame;
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

		bool useSRGB = false;
		GLuint albedoTexure = textureLoader("Textures/pbr_rustediron1/rustediron2_basecolor.png", GL_TEXTURE0, useSRGB); 
		GLuint normalTexture = textureLoader("Textures/pbr_rustediron1/rustediron2_normal.png", GL_TEXTURE1, useSRGB);
		GLuint metalicTexture = textureLoader("Textures/pbr_rustediron1/rustediron2_metallic.png", GL_TEXTURE2, useSRGB);
		GLuint roughnessTexture = textureLoader("Textures/pbr_rustediron1/rustediron2_roughness.png", GL_TEXTURE3, useSRGB);
		

		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();
		shader.setUniform1i("material.albedo", 0);
		shader.setUniform1i("material.normal", 1);
		shader.setUniform1i("material.metalic", 2);
		shader.setUniform1i("material.roughness", 3);
		shader.setUniform1i("material.AO", 4);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

		float lastLightRotationAngle = 0.0f;

		//SphereMeshTextured sphereMesh(1.0f);
		//SphereMeshTextured sphereMesh(0.5f);
		SphereMeshTextured sphereMesh;

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			glPolygonMode(GL_FRONT_AND_BACK, bUseWireFrame ? GL_LINE : GL_FILL);

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);

			//draw light
			lastLightRotationAngle = rotateLight ? 100 * currentTime : lastLightRotationAngle;
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, glm::radians(lastLightRotationAngle), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f));
			glm::vec4 rotatedLightPos = model * glm::vec4(lightPos, 1.0f);

			lampShader.use();
			lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			sphereMesh.render();

			//draw objects
			model = glm::mat4(1.0f);
			model = glm::translate(model, objectPos);
			shader.use();
			shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform3f("lightPosition", rotatedLightPos.x, rotatedLightPos.y, rotatedLightPos.z);
			shader.setUniform3f("objectColor", objectcolor.x, objectcolor.y, objectcolor.z);
			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			sphereMesh.render();

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
	}
}
//
//int main()
//{
//	true_main();
//}