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
#include "../../Utilities/Lighting/DirectionalLight.h"
#include "../../Utilities/Lighting/ConeLight.h"
#include "../../Utilities/Lighting/PointLight.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../ImportingModels/Models/Model.h"
#include <gtc/matrix_transform.hpp>

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
					//fragNormal = mat3(transpose(inverse(model))) * normal;
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

					interpTextCoords = textureCoordinates;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
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
				uniform int toggleFlashLight = 0;
				
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
					lightContribution += CalculateConeLighting(coneLight, normal, toView) * vec3(toggleFlashLight);

					fragmentColor = vec4(lightContribution, 1.0f);
				}

				vec3 CalculateDirectionalLighting(DirectionalLight light, vec3 normal, vec3 toView)
				{
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					vec3 toLight = normalize(-light.direction);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight);
					
					return lightContribution;
				}


				vec3 CalculatePointLighting(PointLight light, vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
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

					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);							

					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
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
	bool toggleFlashLight = false;

	struct Transform
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	struct ModelInstance
	{
		Transform transform;
		Model* modelSource;
	};
	class ColoredPointLight : public PointLight
	{
	public: //accessors
		ColoredPointLight(
			const glm::vec3& ambientIntensity,
			const glm::vec3& diffuseIntensity,
			const glm::vec3& specularIntensity,
			const float attenuationConstant,
			const float attenuationLinear,
			const float attenuationQuadratic,
			const glm::vec3& position,
			std::string uniformName, bool InArray = false, unsigned int arrayIndex = 0) :
			PointLight(ambientIntensity, diffuseIntensity, specularIntensity, attenuationConstant, attenuationLinear, attenuationQuadratic, position, uniformName, InArray, arrayIndex)
		{
		}
		const glm::vec3& getDiffuseColor() { return diffuseIntensity; }
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
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			toggleFlashLight = !toggleFlashLight;
		}

		camera.handleInput(window, deltaTime);
	}

	void true_main()
	{
		camera.setPosition(0.0f, 0.0f, 3.0f);
		int width = 1200;
		int height = 800;

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

		Model meshModel("Models/nanosuit/nanosuit.obj");
		std::vector<ModelInstance> models;
		constexpr float positionOffset = 1.0f;
		constexpr int modelsPerRow = 10;
		constexpr int numModels = 50;
		for (int i = 0, row = 0, col = 0; i < numModels; ++i)
		{
			ModelInstance modelInstance;
			modelInstance.transform.position = glm::vec3(positionOffset * col,0, -positionOffset * row);
			modelInstance.transform.scale = glm::vec3(0.1f);
			modelInstance.modelSource = &meshModel;
			models.emplace_back(std::move(modelInstance));

			if (++col / modelsPerRow != 0)
			{
				row++;
				col = 0;
			}
		}

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

		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.texture_diffuse0", 0);
		shader.setUniform1i("material.texture_specular0", 1);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 0.5f, 2.0f);

		//directional light
		glm::vec3 directionalLightDir(-0.2f, -1.0f, -0.3f);
		glm::vec3 dirAmbientIntensity(0.05f, 0.05f, 0.05f);
		glm::vec3 dirDiffuseIntensity(0.4f, 0.4f, 0.4f);
		glm::vec3 dirSpecularIntensity(0.5f, 0.5f, 0.5f);
		DirectionalLight dirLight(
			dirAmbientIntensity,
			dirDiffuseIntensity,
			dirSpecularIntensity,
			directionalLightDir,
			"dirLight"
		);


		//point lights
		float plghtConstant = 1.f;
		float plghtLinear = 0.09f;
		float plghtQuadratic = 0.032f;

		/*struct PointLight
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
		*/
		glm::vec3 pntAmbient(0.05f, 0.05f, 0.05f);
		glm::vec3 pntDiffuse(0.8f, 0.8f, 0.8f);
		glm::vec3 pntSpecular(1.f, 1.f, 1.f);
		ColoredPointLight pointLights[] = {
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(0.7f, 1.2f, 2.0f),
				"pointLights",true, 0
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(2.3f, 1.3f, -4.0f),
				"pointLights",true, 1
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(4.0f, 2.0f, -6.0f),
				"pointLights",true, 2
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(0.f, 1.0f, -3.f),
				"pointLights",true, 3
			}
		};

		shader.setUniform3f("coneLight.position", camera.getPosition());
		shader.setUniform3f("coneLight.direction", camera.getFront());
		float cosCutOff = glm::cos(glm::radians(12.5f));
		float cosOuterCutOff = glm::cos(glm::radians(17.5f));
		float coneConstant = 1.f;
		float coneLinear = 0.10f;
		float coneQuadratic = 0.065f;
		glm::vec3 coneAmbientIntensity(0.f);
		glm::vec3 coneDiffuseIntensity(1.0f, 1.0f, 1.0f);
		glm::vec3 coneSpecularIntensity(1.0f, 1.0f, 1.0f);
		ConeLight coneLight(
			coneAmbientIntensity,
			coneDiffuseIntensity,
			coneSpecularIntensity,
			cosCutOff,
			cosOuterCutOff,
			coneConstant,
			coneLinear,
			coneQuadratic,
			camera.getPosition(),
			camera.getFront(),
			"coneLight"
		);

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
			glm::vec3 diffuseColor = lightcolor * diffuseStrength;
			glm::vec3 ambientColor = diffuseColor * ambientStrength;

			for (size_t i = 0; i < sizeof(pointLights) / sizeof(ColoredPointLight); ++i)
			{
				glm::mat4 model;
				model = glm::translate(model, pointLights[i].getPosition());
				model = glm::scale(model, glm::vec3(0.2f));

				glm::vec3 lightColor = pointLights[i].getDiffuseColor();

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
			dirLight.applyUniforms(shader);

			//configure point lights
			pointLights[0].applyUniforms(shader);
			pointLights[1].applyUniforms(shader);
			pointLights[2].applyUniforms(shader);
			pointLights[3].applyUniforms(shader);

			//configure coneLight;
			coneLight.setPosition(camera.getPosition());
			coneLight.setDirection(camera.getFront());
			coneLight.applyUniforms(shader);


			//tweak parameters
			shader.setUniform1i("material.shininess", shininess);
			shader.setUniform3f("light.ambientIntensity", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("light.diffuseIntensity", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("light.specularIntensity", lightcolor.x, lightcolor.y, lightcolor.z);
			//shader.setUniform3f("light.specularIntensity", specularStrength, specularStrength, specularStrength);
			shader.setUniform1i("enableAmbient", toggleAmbient);
			shader.setUniform1i("enableDiffuse", toggleDiffuse);
			shader.setUniform1i("enableSpecular", toggleSpecular);

			shader.setUniform1i("toggleFlashLight", toggleFlashLight);

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			for (size_t i = 0; i < models.size(); ++i)
			{
				Transform& transform = models[i].transform;
				
				glm::mat4 model = glm::mat4(1.f); //set model to identity matrix
				model = glm::translate(model, transform.position);
				model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
				model = glm::scale(model, transform.scale);
				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));

				models[i].modelSource->draw(shader);
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