#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"
#include "ReferenceCode/OpenGL/InputTracker.h"

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
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);

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

				struct PointLight
				{
					vec3 position;

					//intensities are like strengths -- but they may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;

					//attenuation constants
					float constant;
					float linear;
					float quadratic;
				};	
				#define NUM_PNT_LIGHTS 4
				uniform PointLight pointLights[NUM_PNT_LIGHTS];

				struct DirectionalLight
				{
					vec3 direction;
					//intensities are strengths, but may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;
				};
				uniform DirectionalLight dirLight;

				struct ConeLight 
				{
					float cos_cutoff;
					float cos_outercutoff;
					vec3 position;
					vec3 direction;

					//intensities are strengths, but may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;	
	
					//attenuation constants
					float constant;
					float linear;
					float quadratic;			
				};
				uniform ConeLight coneLight;

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;
				
				//function forward declarations
				vec3 CalculateDirectionalLighting(DirectionalLight light, vec3 normal, vec3 toView);
				vec3 CalculateConeLighting(ConeLight light, vec3 normal, vec3 toView);
				vec3 CalculatePointLighting(PointLight light, vec3 normal, vec3 toView, vec3 fragPosition);

				void main(){
					vec3 normal = normalize(fragNormal);														
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 lightContribution = vec3(0.f);

					lightContribution += CalculateDirectionalLighting(dirLight, normal, toView);					
					for(int i = 0; i < NUM_PNT_LIGHTS; ++i)
					{
						lightContribution += CalculatePointLighting(pointLights[i], normal, toView, fragPosition);
					}
					lightContribution += CalculateConeLighting(coneLight, normal, toView);

					fragmentColor = vec4(lightContribution, 1.0f);
				}

				vec3 CalculateDirectionalLighting(DirectionalLight light, vec3 normal, vec3 toView)
				{
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	

					vec3 toLight = normalize(-light.direction);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight);
					
					return lightContribution;
				}


				vec3 CalculatePointLighting(PointLight light, vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation

					return lightContribution;
				}
				vec3 CalculateConeLighting(ConeLight light, vec3 normal, vec3 toView)
				{ 
					vec3 toLight = normalize(light.position - fragPosition);
					float cosTheta = dot(-toLight, normalize(light.direction));
					
					//note light.cutoff will have a higher value than light.outercutoff because they're cos(angle) values; Smaller angles will be closer to 1.
					float epsilon = light.cos_cutoff - light.cos_outercutoff;
					float intensity = (cosTheta - light.cos_outercutoff) / epsilon;
					intensity = clamp(intensity, 0, 1);

					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);							

					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					//allow ambient light regardless of spotlight cone
					vec3 lightContribution = ambientLight + (diffuseLight + specularLight) * intensity;
					lightContribution *= attenuation;					

					return lightContribution;
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


	float yRotation = 0.f;
	bool rotateLight = true;
	bool toggleAmbient = true;
	bool toggleDiffuse = true;
	bool toggleSpecular = true;
	float ambientStrength = 0.2f;
	float diffuseStrength = 0.5f;
	float specularStrength = 1.f;
	int shininess = 32;
	float floatValIncrement = 0.25f;

	struct PointLight
	{
		glm::vec3 position;
		glm::vec3 ambientIntensity;
		glm::vec3 diffuseIntensity;
		glm::vec3 specularIntensity;
		//attenuation constants
		float constant;
		float linear;
		float quadratic;
	};

	void processInput(GLFWwindow* window)
	{
		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

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
		GLuint diffuseMap = textureLoader("Textures/container2.png", GL_TEXTURE0);
		GLuint specularMap = textureLoader("Textures/container2_specular.png", GL_TEXTURE1);

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.diffuseMap", 0);
		shader.setUniform1i("material.specularMap", 1);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 0.5f, 2.0f);

		glm::vec3 cubePositions[] = {
			glm::vec3(0.0f,  0.0f,  0.0f),
			glm::vec3(2.0f,  5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3(2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f,  3.0f, -7.5f),
			glm::vec3(1.3f, -2.0f, -2.5f),
			glm::vec3(1.5f,  2.0f, -2.5f),
			glm::vec3(1.5f,  0.2f, -1.5f),
			glm::vec3(-1.3f,  1.0f, -1.5f)
		};

		//directional light
		glm::vec3 directionalLightDir(-0.2f, -1.0f, -0.3f);
		glm::vec3 dirAmbientIntensity(0.05f, 0.05f, 0.05f);
		glm::vec3 dirDiffuseIntensity(0.4f, 0.4f, 0.4f);
		glm::vec3 dirSpecularIntensity( 0.5f, 0.5f, 0.5f);

		//point lights
		float plghtConstant = 1.f;
		float plghtLinear = 0.09f;
		float plghtQuadratic = 0.032f;
		PointLight pointLights[] = {
			{
				glm::vec3(0.7f, 0.2f, 2.0f),
				glm::vec3(0.05f, 0.05f, 0.05f),
				glm::vec3(0.8f, 0.8f, 0.8f),
				glm::vec3(1.f, 1.f, 1.f),
				plghtConstant,
				plghtLinear,
				plghtQuadratic
			},
			{
				glm::vec3(2.3f, -3.3f, -4.0f),
				glm::vec3(0.05f, 0.05f, 0.05f),
				glm::vec3(0.8f, 0.8f, 0.8f),
				glm::vec3(1.f, 1.f, 1.f),
				plghtConstant,
				plghtLinear,
				plghtQuadratic
			},
			{
				glm::vec3(-4.0f, 2.0f, -12.0f),
				glm::vec3(0.05f, 0.05f, 0.05f),
				glm::vec3(0.8f, 0.8f, 0.8f),
				glm::vec3(1.f, 1.f, 1.f),
				plghtConstant,
				plghtLinear,
				plghtQuadratic
			},
			{
				glm::vec3(0.f, 0.f, -3.f),
				glm::vec3(0.05f, 0.05f, 0.05f),
				glm::vec3(0.8f, 0.8f, 0.8f),
				glm::vec3(1.f, 1.f, 1.f),
				plghtConstant,
				plghtLinear,
				plghtQuadratic
			}
		};


		while (!glfwWindowShouldClose(window))
		{
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
			glm::vec3 ambientColor = diffuseColor * ambientStrength; 

			for (size_t i = 0; i < sizeof(pointLights) / sizeof(PointLight); ++i)
			{
				glm::mat4 model;
				model = glm::translate(model, pointLights[i].position);
				model = glm::scale(model, glm::vec3(0.2f));

				lampShader.use();
				lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
				glBindVertexArray(lampVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}


			//draw objects
			shader.use();

			//configure directional lighting
			shader.setUniform3f("dirLight.direction", directionalLightDir);
			shader.setUniform3f("dirLight.ambientIntensity", dirAmbientIntensity);
			shader.setUniform3f("dirLight.diffuseIntensity", dirDiffuseIntensity);
			shader.setUniform3f("dirLight.specularIntensity", dirSpecularIntensity);

			//configure point lights
			shader.setUniform3f("pointLights[0].position"				, pointLights[0].position);
			shader.setUniform3f("pointLights[0].ambientIntensity"		, pointLights[0].ambientIntensity);
			shader.setUniform3f("pointLights[0].diffuseIntensity"		, pointLights[0].diffuseIntensity);
			shader.setUniform3f("pointLights[0].specularIntensity"		, pointLights[0].specularIntensity);
			shader.setUniform1f("pointLights[0].constant"				, pointLights[0].constant);
			shader.setUniform1f("pointLights[0].linear"					, pointLights[0].linear);
			shader.setUniform1f("pointLights[0].quadratic"				, pointLights[0].quadratic);

			shader.setUniform3f("pointLights[1].position",				pointLights[1].position);
			shader.setUniform3f("pointLights[1].ambientIntensity",		pointLights[1].ambientIntensity);
			shader.setUniform3f("pointLights[1].diffuseIntensity",		pointLights[1].diffuseIntensity);
			shader.setUniform3f("pointLights[1].specularIntensity",		pointLights[1].specularIntensity);
			shader.setUniform1f("pointLights[1].constant",				pointLights[1].constant);
			shader.setUniform1f("pointLights[1].linear",				pointLights[1].linear);
			shader.setUniform1f("pointLights[1].quadratic",				pointLights[1].quadratic);

			shader.setUniform3f("pointLights[2].position",				pointLights[2].position);
			shader.setUniform3f("pointLights[2].ambientIntensity",		pointLights[2].ambientIntensity);
			shader.setUniform3f("pointLights[2].diffuseIntensity",		pointLights[2].diffuseIntensity);
			shader.setUniform3f("pointLights[2].specularIntensity",		pointLights[2].specularIntensity);
			shader.setUniform1f("pointLights[2].constant",				pointLights[2].constant);
			shader.setUniform1f("pointLights[2].linear",				pointLights[2].linear);
			shader.setUniform1f("pointLights[2].quadratic",				pointLights[2].quadratic);

			shader.setUniform3f("pointLights[3].position",				pointLights[3].position);
			shader.setUniform3f("pointLights[3].ambientIntensity",		pointLights[3].ambientIntensity);
			shader.setUniform3f("pointLights[3].diffuseIntensity",		pointLights[3].diffuseIntensity);
			shader.setUniform3f("pointLights[3].specularIntensity",		pointLights[3].specularIntensity);
			shader.setUniform1f("pointLights[3].constant",				pointLights[3].constant);
			shader.setUniform1f("pointLights[3].linear",				pointLights[3].linear);
			shader.setUniform1f("pointLights[3].quadratic",				pointLights[3].quadratic);

			//configure coneLight;
			shader.setUniform3f("coneLight.position", camera.getPosition());
			shader.setUniform3f("coneLight.direction", camera.getFront());
			shader.setUniform1f("coneLight.cos_cutoff", glm::cos(glm::radians(12.5f)));
			shader.setUniform1f("coneLight.cos_outercutoff", glm::cos(glm::radians(17.5f)));
			shader.setUniform1f("coneLight.constant", 1);
			shader.setUniform1f("coneLight.linear", 0.10f);
			shader.setUniform1f("coneLight.quadratic", 0.065f);
			shader.setUniform3f("coneLight.ambientIntensity", glm::vec3(0.f));
			shader.setUniform3f("coneLight.diffuseIntensity", 1.0f, 1.0f, 1.0f);
			shader.setUniform3f("coneLight.specularIntensity", 1.0f, 1.0f, 1.0f);


			//tweak parameters
			shader.setUniform1i("material.shininess", shininess);
			shader.setUniform3f("light.ambientIntensity", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("light.diffuseIntensity", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("light.specularIntensity", lightcolor.x, lightcolor.y, lightcolor.z);
			//shader.setUniform3f("light.specularIntensity", specularStrength, specularStrength, specularStrength);
			shader.setUniform1i("enableAmbient", toggleAmbient);
			shader.setUniform1i("enableDiffuse", toggleDiffuse);
			shader.setUniform1i("enableSpecular", toggleSpecular);

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);

			for (size_t i = 0; i < sizeof(cubePositions) / sizeof(glm::vec3); ++i)
			{
				float angle = 20.0f * i;
				glm::mat4 model(1.f); //set model to identity matrix
				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

				glBindVertexArray(vao);
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

//int main()
//{
//	true_main();
//}