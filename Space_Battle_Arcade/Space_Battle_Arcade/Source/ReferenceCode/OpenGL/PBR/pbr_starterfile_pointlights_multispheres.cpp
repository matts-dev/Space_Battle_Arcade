#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../nu_utils.h"
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

				uniform vec3 albedo_u;
				uniform float metalic_u;
				uniform float roughness_u;
				uniform float AO_u = 0.0f;

				uniform bool bUseTextureData = false;
				

				void main(){
					vec3 albedo;
					float roughness;
					float metalness;
					float AO;
					vec3 normal;
					
					if(bUseTextureData){
						albedo = texture(material.albedo,texCoords).rgb;
						//normal = texture(material.normal,texCoords).rgb; 
						normal = normalize(fragNormal); 
						metalness = texture(material.metalic,texCoords).r;
						roughness = texture(material.roughness,texCoords).r;
						AO = texture(material.AO,texCoords).r;
					}
					else {
						albedo = albedo_u;
						metalness = metalic_u;
						roughness = roughness_u;
						AO = AO_u;
						normal = normalize(fragNormal); 
					}

					vec3 ambientLight = ambientStrength * lightColor;					

					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * lightColor;

					float specularStrength = 0.5f;
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 toReflection = reflect(-toView, normal); //reflect expects vector from light position (tutorial didn't normalize this vector)
					float specularAmount = pow(max(dot(toReflection, toLight), 0), 32);
					vec3 specularLight = specularStrength * lightColor * specularAmount;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * albedo;
					
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

	struct SphereData
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		float metalness;
		float roughness;
		glm::vec3 albedo;
		float ao; //ambient occlusion factor
	};

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
		glm::vec3 lightPos(1.2f, 5.0f, 2.0f);

		float lastLightRotationAngle = 0.0f;

		//SphereMeshTextured sphereMesh(1.0f);
		//SphereMeshTextured sphereMesh(0.5f);
		SphereMeshTextured sphereMesh;

		std::vector<SphereData> sphereMeshes;
		int numRows = 7;
		int numCols = 7;
		float spacing = 2.5f;
		float yStart = 5.f;
		float xStart = -numCols * spacing / 2;
		for (int row = 0; row < numRows; ++row)
		{
			for (int col = 0; col < numCols; ++col)
			{
				sphereMeshes.push_back(SphereData{});
				int index = row * numCols + col;

				auto& inserted = sphereMeshes[index];

				float roughness = col / static_cast<float>(col);
				inserted.roughness = glm::clamp(roughness, 0.05f, 1.0f);
				inserted.metalness = row / static_cast<float>(numRows);
				inserted.position = glm::vec3(col*spacing + xStart, -row * spacing + yStart, -5);
				inserted.albedo = glm::vec3(0.5f, 0.0f, 0.0f);
				inserted.ao = 1.0f;
			}
		}

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
			model = glm::rotate(model, glm::radians(lastLightRotationAngle), glm::vec3(0, 0, 1)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f));
			glm::vec4 rotatedLightPos = model * glm::vec4(lightPos, 1.0f);

			lampShader.use();
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			sphereMesh.render();

			//draw objects
			shader.use();
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform3f("lightPosition", rotatedLightPos.x, rotatedLightPos.y, rotatedLightPos.z);
			shader.setUniform3f("objectColor", objectcolor.x, objectcolor.y, objectcolor.z);
			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);

			for (unsigned int idx = 0; idx < sphereMeshes.size(); ++idx)
			{
				model = glm::mat4(1.0f);
				model = glm::translate(model, sphereMeshes[idx].position);
				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader.setUniform1f("metalness_u", sphereMeshes[idx].metalness);
				shader.setUniform1f("roughness_u", sphereMeshes[idx].roughness);
				shader.setUniform3f("albedo_u", sphereMeshes[idx].albedo);
				shader.setUniform1f("ao_u", sphereMeshes[idx].ao);
				sphereMesh.render();
			}

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
	}
}

//int main()
//{
//	true_main();
//}