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
#include <thread>
#include <chrono>

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

				uniform mat4 shadowSpaceTransform;

				out vec3 fragNormal;
				out vec3 fragPosition;
				out vec2 interpTextCoords;
				out vec4 shadowSpaceFragPosition;

				void main(){
					/*
					vec4 modelPosition = model * vec4(position, 1);

					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);
					fragPosition = vec3(modelPosition);
					shadowSpaceFragPosition =  shadowSpaceTransform * modelPosition;
					gl_Position = projection * view * model * vec4(position, 1);					

					*/
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);

					shadowSpaceFragPosition = shadowSpaceTransform * model * vec4(position, 1);
					//shadowSpaceFragPosition = shadowSpaceFragPosition / shadowSpaceFragPosition.w; //this may need to be done in frag shader; at least for point shadows since they use the geometry shader in some way (haven't learned those yet)

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
					vec3 direction;
					
					//intensities are like strengths -- but they may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;
				};	
				uniform Light light;

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;
				in vec4 shadowSpaceFragPosition;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;

				uniform sampler2D shadowDepthCapture;

				float isInShadow(){
					//transform [-1, 1] range to [0, 1] range for texture sampling and depth comparisons.
					vec4 rangeTransformed = shadowSpaceFragPosition;					
					rangeTransformed = (rangeTransformed / 2.0f) + 0.5f;
					
					float currentFragDepth = rangeTransformed.z;

					vec3 toLight = normalize(-light.direction);
					vec3 flatNormal = normalize(fragNormal); //fragNormal is NOT the flat normal, it is my impression that it should use flat normals though, but I haven't tested with smooth shading.
					float bias = (1.0f - dot(toLight, flatNormal)) * 0.05;
					bias = max(0.005, bias);

					//non-PCF
					//float depthOfFirstShadow = texture(shadowDepthCapture, rangeTransformed.xy).r;
					//float shadow = ((currentFragDepth - bias) > depthOfFirstShadow) ? 1.0f : 0.0f;
					
					//simple PCF method
					vec2 texelSize = 1.0f / textureSize(shadowDepthCapture, 0);
					float shadow = 0.0f;
					for(int x = -1; x <= 1; ++x){
						for(int y = -1; y <= 1; ++y){
							float texelShadowDepth = texture(shadowDepthCapture, rangeTransformed.xy + texelSize.xy * vec2(x, y)).r;
							shadow += ((currentFragDepth - bias) > texelShadowDepth ) ? 1.0f : 0.0f;
						}
					}
					shadow /= 9;

					if(currentFragDepth > 1)
						shadow = 0;

					return shadow;
				}

				void main(){
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	

					vec3 toLight = normalize(-light.direction);
					vec3 normal = normalize(fragNormal);											
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												
					vec3 toView = normalize(cameraPosition - fragPosition);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					vec3 lightContribution = ambientLight + (1.0f - isInShadow() ) * (diffuseLight + specularLight);
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

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";

	const char* dir_shadow_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* dir_shadow_frag_shader_src = R"(
				#version 330 core

				void main(){
					//gl_FragDepth = gl_FragCoord.z;
				}
			)";

	const char* quad_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec2 position;				
				layout (location = 1) in vec2 texCoords;				
				
				out vec2 interpTexCoords;

				void main(){
					gl_Position = vec4(position, 0, 1);
					interpTexCoords = texCoords;
				}
			)";
	const char* quad_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;

				uniform sampler2D screenCapture;
				
				void main(){
					fragmentColor = texture(screenCapture, interpTexCoords);

					//make depth appear white instead of read
					fragmentColor = vec4(fragmentColor.rrr, 1.0);
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
	
//--------------------------------------------------------------------------------------------
	glm::vec3 shadowCamPos = glm::vec3(-2, 4, -1.0f);
	glm::vec3 shadowCamTarget = glm::vec3(0, 0, 0);
	bool bDisplayDepthView = false;
	bool bEnableCullFace = false;
	float lightRot_z = 45.0f; //give it a nudge so glm::look_at doesn't give back nan's
	float lightRot_x = 0.0f;
	const float rotationSensitivity = 100.0f;
	const float depthIncrement = 5.0f;
	glm::vec3 lightBoomStickLoc = glm::vec3(0, 4.0f, 0);
	float shadowFarDepth = 10.0f;

//--------------------------------------------------------------------------------------------

	void processInput(GLFWwindow* window, float deltaTime = 0.0f)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press F to switch to depth framebuffer" << std::endl
				<< "Press UP/DOWN/LEFT/RIGHT to move light." << std::endl
				<< " Press Q To lower shadow far plane, press R to increase shadow far plane" << std::endl
				<< std::endl; 
			return 0; 
		}();
		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

		//--------------------------------------------------------------------------------------------
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			bDisplayDepthView = !bDisplayDepthView;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_C))
		{
			bEnableCullFace = !bEnableCullFace;

			if (bEnableCullFace)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			lightRot_x += -rotationSensitivity * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			lightRot_x += rotationSensitivity * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			lightRot_z += rotationSensitivity * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			lightRot_z += -rotationSensitivity * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		{
			shadowFarDepth += deltaTime * depthIncrement;
		}
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			shadowFarDepth -= deltaTime * depthIncrement;
		}

		//--------------------------------------------------------------------------------------------


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

	struct Transform
	{
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	void RenderCube(Transform& transform, Shader& shader, GLuint vao, glm::mat4& projection, glm::mat4& view)
	{
		glm::mat4 model = glm::mat4(1.f); //set model to identity matrix
		model = glm::translate(model, transform.position);
		model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
		model = glm::scale(model, transform.scale);

		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	void RenderQuad(const Transform& transform, Shader& shader, GLuint vao, const glm::mat4& projection, const glm::mat4& view)
	{
		glm::mat4 model = glm::mat4(1.f); //set model to identity matrix
		model = glm::translate(model, transform.position);
		model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
		model = glm::scale(model, transform.scale);

		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
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

		//----------------------------- CUBE VAO SET UP -------------------------------------------
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
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

		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//----------------------------- QUAD VAO SET UP -------------------------------------------
		float quadVerts[] = {
			//x    y         s    t
			-1.0, -1.0,		0.0, 0.0,
			1.0, -1.0,		1.0, 0.0,
			-1.0, 1.0,		0.0, 1.0,

			-1.0, 1.0,      0.0, 1.0,
			1.0, -1.0,      1.0, 0.0,
			1.0, 1.0,		1.0, 1.0
		};
		GLuint postVAO;
		glGenVertexArrays(1, &postVAO);
		glBindVertexArray(postVAO);

		GLuint postVBO;
		glGenBuffers(1, &postVBO);
		glBindBuffer(GL_ARRAY_BUFFER, postVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		//---------------------------------------------------------------------------

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

		Shader shadowMapShader(dir_shadow_vertex_shader_src, dir_shadow_frag_shader_src, false);
		
		Shader QuadViewingShader(quad_vertex_shader_src, quad_frag_shader_src, false);


		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 0.5f, 2.0f);
		glm::vec3 lightDirection(-0.2f, -1.0f, -0.3f);

		//Transform cubeTransforms[] = {
		//	//pos					rot        scale
		//	{ { 0,0,0 },	{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
		//	{ { 2,2,0 },		{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
		//	{ { 2,0,2 },	{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
		//	{ { -2,0,-2 },	{ 0, 45, 0 },{ 0.5f,0.5f,0.5f } }
		//};
		Transform floorTransform = { { 0,-1,0 },{ 0, 0, 0 },{ 25, 0.5f, 25 } };
		Transform testQuadTransform = { {0,0,-3}, {0, 35, 0}, {1, 1, 1} };

		Transform cubeTransforms[] = {
			//pos					rot        scale
			{ { 0,1.5f,0 },			{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
			{ { 2,0.1f,1.0f },		{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
			{ { -1,0,2 },			{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
			{ { -2.5f,0,-0.1f },	{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } }
		};

		//--------------------------------------------------------------------------------------------
		GLuint shadowFBO;
		glGenFramebuffers(1, &shadowFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

		GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; 
		GLuint shadowDepthAttachment;
		glGenTextures(1, &shadowDepthAttachment);
		glBindTexture(GL_TEXTURE_2D, shadowDepthAttachment); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
		float borderColor[] = { 1, 1, 1, 1 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthAttachment, 0); //*
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "FAILURE TO CREATE SHADOW MAP FRAME BUFFER" << std::endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//--------------------------------------------------------------------------------------------
		

		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window, deltaTime);

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);
			//-----------------------------------------------------------------------------------------
			glm::mat4 lightLocal(1.0f);
			lightLocal = glm::translate(lightLocal, lightBoomStickLoc);

			glm::mat4 lightParentModel(1.0f);
			lightParentModel = glm::rotate(lightParentModel, glm::radians(lightRot_x), glm::vec3(1, 0, 0));
			lightParentModel = glm::rotate(lightParentModel, glm::radians(lightRot_z), glm::vec3(0, 0, 1));

			shadowCamPos = lightParentModel * lightLocal * glm::vec4(0, 0, 0, 1);
			lightDirection = shadowCamTarget - shadowCamPos;

			//-----------------------------------------------------------------------------------------

			//--------------------------------------------------------------------------------------------
			glm::mat4 shadowView = glm::lookAt(shadowCamPos, shadowCamTarget, glm::vec3(0, 1, 0));

			float ortho_width = 20;
			float ortho_height = 20;
			float left, right, top, bottom, near_val, far_val;
			left = -ortho_width / 2;
			right = ortho_width / 2;
			top = ortho_height / 2;
			bottom = -ortho_height / 2;
			near_val = 1.0f;
			//far_val = 7.5f;
			far_val = shadowFarDepth;
			glm::mat4 shadowProjection = glm::ortho(left, right, bottom, top, near_val, far_val);
			glm::mat4 shadowSpaceTransform = shadowProjection * shadowView;
			//--------------------------------------------------------------------------------------------

			//--------------------------------------------------------------------------------------------
			if (bEnableCullFace)
				glCullFace(GL_FRONT);

			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glClear(GL_DEPTH_BUFFER_BIT);

			shadowMapShader.use();
			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shadowMapShader, vao,               shadowProjection, shadowView);
			}
			RenderCube(floorTransform, shadowMapShader, vao,                      shadowProjection, shadowView);
			RenderQuad(testQuadTransform, shadowMapShader, postVAO,               shadowProjection , shadowView);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, width, height);

			if (bEnableCullFace)
				glCullFace(GL_BACK);
			//--------------------------------------------------------------------------------------------

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::vec3 diffuseColor = lightcolor * diffuseStrength;
			glm::vec3 ambientColor = diffuseColor * ambientStrength;
			shader.use();
			shader.setUniform1i("material.shininess", shininess);
			shader.setUniform3f("light.ambientIntensity", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("light.diffuseIntensity", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("light.specularIntensity", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform1i("enableAmbient", toggleAmbient);
			shader.setUniform1i("enableDiffuse", toggleDiffuse);
			shader.setUniform1i("enableSpecular", toggleSpecular);
			//---------------------------------
			glm::vec3 lightDirection = shadowCamTarget - shadowCamPos;
			shader.setUniform3f("light.direction", lightDirection.x, lightDirection.y, lightDirection.z);
			//--------------------------------------------------------------------------------------------
			shader.setUniform1i("shadowDepthCapture", 5);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, shadowDepthAttachment);
			shader.setUniformMatrix4fv("shadowSpaceTransform", 1, GL_FALSE, glm::value_ptr(shadowSpaceTransform));
			//--------------------------------------------------------------------------------------------
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, specularMap);

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);

			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shader, vao, projection, view);
			}
			RenderCube(floorTransform, shader, vao, projection, view);
			RenderQuad(testQuadTransform, shader, postVAO, projection, view);

			if (bDisplayDepthView)
			{
				QuadViewingShader.use();
				QuadViewingShader.setUniform1i("screenCapture", 5);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, shadowDepthAttachment);
				RenderQuad(Transform(), QuadViewingShader, postVAO, glm::mat4(), glm::mat4());
			}

			glfwSwapBuffers(window);
			glfwPollEvents();


			using namespace std::chrono_literals;
			std::this_thread::sleep_for(16ms);
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