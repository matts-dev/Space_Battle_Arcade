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
#include <cstdint>

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
				uniform bool bInvertNormals = false;
				uniform bool bDisableAttenuation = false;

				uniform samplerCube lightDepthMap;
				uniform float lightFarPlaneDepth;

				const int NUM_SAMPLES = 20;
				vec3 sampleOffsetDirections[NUM_SAMPLES] = vec3[]
				(
				   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
				   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
				   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
				   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
				   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
				);   

				float calculateShadow() {
		
				//simple hard shadows
				/*
					vec3 toFragFromLight = fragPosition - light.position;

					float lightBlockedDepth = texture(lightDepthMap, toFragFromLight).r;
					lightBlockedDepth = lightBlockedDepth * lightFarPlaneDepth; //[0, 1] -> [0, farPlane]
					float fragDepthFromLight = length(toFragFromLight);					

					float bias = 0.05f;

					float shadow = fragDepthFromLight - bias > lightBlockedDepth ? 1.0f : 0.0f;
				*/

				//expensive soft shadows my version
				/*
					vec3 toFragFromLight = fragPosition - light.position;
					float bias = 0.05f;
					float samples = 4;
					float sampleOffset = 0.05f;
					float increment = sampleOffset / samples;

					float startAroundZero = increment / 2.0f;
					float sampleHalfRange = ((samples / 2.0) - 1) * increment;
					float sampleStart = sampleHalfRange + startAroundZero;
					

					float shadow = 0;
					for(float x = -sampleStart; x <= sampleStart; x += increment){
						for(float y = -sampleStart; y <= sampleStart; y += increment){
							for(float z = -sampleStart; z <= sampleStart; z += increment){
								vec3 sampleVector = toFragFromLight + vec3(x, y, z);
								float lightBlockedDepth = texture(lightDepthMap, sampleVector).r;

								lightBlockedDepth = lightBlockedDepth * lightFarPlaneDepth; //[0, 1] -> [0, farPlane]
								float fragDepthFromLight = length(sampleVector );													

								shadow += fragDepthFromLight - bias > lightBlockedDepth ? 1.0f : 0.0f;
							}
						}
					}
					shadow /= samples * samples * samples;
				*/
					
				//expensive shadows tutorial version
				/*
					vec3 toFragFromLight = fragPosition - light.position;
					float bias = 0.05f;
					float samples = 4;
					float sampleOffset = 0.1F;
					float sampleInc = sampleOffset / (samples * 0.5);

					float shadow = 0;
					for(float x = -sampleOffset ; x < sampleOffset ; x += sampleInc){
						for(float y = -sampleOffset ; y < sampleOffset ; y += sampleInc){
							for(float z = -sampleOffset ; z < sampleOffset ; z += sampleInc){
								vec3 sampleVector = toFragFromLight + vec3(x, y, z);
								float lightBlockedDepth = texture(lightDepthMap, sampleVector).r;

								lightBlockedDepth = lightBlockedDepth * lightFarPlaneDepth; //[0, 1] -> [0, farPlane]
								float fragDepthFromLight = length(sampleVector );													

								shadow += fragDepthFromLight - bias > lightBlockedDepth ? 1.0f : 0.0f;
							}
						}
					}
					shadow /= samples * samples * samples;
				*/
				//soft shadows using separable offsetse
					vec3 toFragFromLight = fragPosition - light.position;
					float distance = length(toFragFromLight);
					float bias = 0.15f; //0.05f;

					//should keep this lower than the bias
					//don't want the sample radius to reach behind the the cube (into darkness)
					//float sampleRadius = 0.01f; 
					float sampleRadius = min( bias / 5.0f, (1.0 + (distance / lightFarPlaneDepth)) / lightFarPlaneDepth);
					

					float shadow = 0;
					for(int i = 0; i < NUM_SAMPLES; ++i){
						vec3 sampleVector = toFragFromLight + sampleRadius * sampleOffsetDirections[i];
						float lightBlockedDepth = texture(lightDepthMap, sampleVector).r;

						lightBlockedDepth = lightBlockedDepth * lightFarPlaneDepth; //[0, 1] -> [0, farPlane]
						float fragDepthFromLight = length(sampleVector );													

						shadow += fragDepthFromLight - bias > lightBlockedDepth ? 1.0f : 0.0f;
					}
					shadow /= NUM_SAMPLES;

					return shadow;
				}

				void main(){
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	//diffuse color is generally similar to ambient color

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 normal = normalize(fragNormal);														//interpolation of different normalize will cause loss of unit length
					normal = bInvertNormals ? -normal : normal;

					vec3 diffuseLight = max(dot(toLight, normal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												//reflect expects vector from light position (tutorial didn't normalize this vector)
					vec3 toView = normalize(cameraPosition - fragPosition);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
					attenuation = bDisableAttenuation ? 1 : attenuation;

					float isLitAmount = 1.0f - calculateShadow();

					vec3 lightContribution = (ambientLight + isLitAmount * (diffuseLight + specularLight)) * vec3(attenuation);
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

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";
	const char* shadowmap_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;

				void main(){
					gl_Position = model * vec4(position, 1);
				}
			)";
	const char* shadowmap_geometry_shader_src = R"(
				#version 330 core
				layout (triangles) in;
				layout (triangle_strip, max_vertices=18) out;

				uniform mat4 lightTransforms[6];

				out vec4 fragPos;
				
				void main(){
					for(int face = 0; face < 6; ++face){
						for(int vertex = 0; vertex < 3; ++vertex)
						{
							//select face of cubemap to render to.
							gl_Layer = face;
						
							fragPos = gl_in[vertex].gl_Position;
							gl_Position = lightTransforms[face] * fragPos;
							EmitVertex();
						}
						EndPrimitive();
						
					}					

				}
			)";
	const char* shadowmap_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec4 fragPos;
			
				uniform vec4 lightWorldPos;
				uniform float farPlane;
				
				void main(){
					vec4 toFrag = fragPos - lightWorldPos;
					
					gl_FragDepth = length(toFrag) / farPlane;
				}
			)";

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

	float yRotation = 0.f;
	bool rotateLight = true;
	bool toggleAmbient = true;
	bool toggleDiffuse = true;
	bool toggleSpecular = true;
	float ambientStrength = 0.2f;// 0.2f;
	float diffuseStrength = 0.5f;// 0.5f;
	float specularStrength = 1.f;
	int shininess = 64;
	float floatValIncrement = 0.25f;
	//--------------------------------
	bool bDisableAttenuation = true;
	//----------------------------------

	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press F to switch to depth framebuffer" << std::endl
				<< "Press R to toggle light rotation" << std::endl
				<< "Press T to enable light falloff attenuation" << std::endl
				<< "" << std::endl
				<< "" << std::endl
				<< "" << std::endl
				<< std::endl;
			return 0;
		}();

		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

//--------------------------------------------------
		if (input.isKeyJustPressed(window, GLFW_KEY_T))
		{
			bDisableAttenuation = !bDisableAttenuation;
		}
//--------------------------------------------------
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

		//before unbinding any buffers, make sure VAO isn't recording state.
		glBindVertexArray(0);

		//GENERATE LAMP
		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//textures
		GLuint diffuseMap = textureLoader("Textures/woodfloor.png", GL_TEXTURE0);
		//GLuint specularMap = textureLoader("Textures/white.png", GL_TEXTURE1);
		GLuint specularMap = textureLoader("Textures/woodfloor.png", GL_TEXTURE1);


		//--------------------------------------------------------------------------------------------
		// depth map for shadows
		GLuint shadowWidth = 1024;
		GLuint shadowHeight = 1024;
		GLuint shadowCubeMap;
		glGenTextures(1, &shadowCubeMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap);
		for (uint32_t offset = 0; offset < 6; ++offset)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + offset, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		//framebuffer for shadowmap rendering
		GLuint shadowFBO;
		glGenFramebuffers(1, &shadowFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowCubeMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "failed to create the shadow map FBO" << std::endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//--------------------------------------------------------------------------------------------

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.diffuseMap", 0);
		shader.setUniform1i("material.specularMap", 1);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		Shader shadowMapShader(shadowmap_vertex_shader_src, shadowmap_frag_shader_src, shadowmap_geometry_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 0.0f, 2.0f);
		glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

		glm::vec3 cubePositions[] = {
			glm::vec3(4.0f, -3.5f, 0.0),
			glm::vec3(2.0f, 3.0f, 1.0),
			glm::vec3(-3.0f, -1.0f, 0.0),
			glm::vec3(-1.5f, 1.0f, 1.5),
			glm::vec3(-1.5f, 2.0f, -3.0),
			glm::vec3(-1.7f,  3.0f, -7.5f)
		};

		Transform cubeTransforms[] = {
			//pos					rot        scale
			{ { 4.0f, -3.5f, 0.0f },{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
			{ { 2.0f, 3.0f, 1.0f },{ 0, 0, 0 },{ 0.75f,0.75f,0.75f } },
			{ { -3.0f, -1.0f, 0.0f },{ 0, 0, 0 },{ 0.5f,0.5f,0.5f } },
			{ { -1.5f, 1.0f, 1.5f},{ 0, 45, 0 },{ 0.5f,0.5f,0.5f } },
			{ { -1.5f, 2.0f, -3.0f },{ 60, 0, 60 },{ 0.75f,0.75f,0.75f } }
		};

		Transform BoundingBox = { { 0.0f, 0.0f, 0.0f },{ 0, 0, 0 },{ 5.0f,5.0f,5.0f } };

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

			glm::vec3 diffuseColor = lightcolor * diffuseStrength;
			glm::vec3 ambientColor = diffuseColor * ambientStrength; //this is the tutorial did, seems like we should use lightcolor instead of diffuseColor.

			glm::mat4 model;
			model = glm::rotate(model, glm::radians(rotateLight ? 100 * currentTime : 0), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightStart);
			model = glm::scale(model, glm::vec3(0.2f));
			lightPos = glm::vec3(model * glm::vec4(0, 0, 0, 1));

			lampShader.use();
			lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			glBindVertexArray(lampVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			//--------------------------------------------------------------------------------------------
			float lightFarPlaneDistance = 25.0f;

			glm::mat4 lightProjectionMatrix = glm::perspective(glm::radians(90.0f), shadowWidth / (float)shadowHeight, 0.1f, lightFarPlaneDistance);

			std::vector<glm::mat4> lightViewMatrices;
			//ordering will need to match ordering of GL_TEXTURE_CUBE_MAP_POSITIVE_X
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));	//+x
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)));	//-x
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));	//+y
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)));	//-y
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));	//+z
			lightViewMatrices.push_back(glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)));	//-z

			//prepare shadow map shader's uniforms
			shadowMapShader.use();
			for (int face = 0; face < 6; ++face)
			{
				std::string uniformName = ("lightTransforms[" + std::to_string(face) + "]");
				shadowMapShader.setUniformMatrix4fv(uniformName.c_str(), 1, GL_FALSE, glm::value_ptr(lightProjectionMatrix * lightViewMatrices[face]));
			}
			shadowMapShader.setUniform1f("farPlane", lightFarPlaneDistance);
			shadowMapShader.setUniform4f("lightWorldPos", glm::vec4(lightPos, 1.0f));
			
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
			glViewport(0, 0, shadowWidth, shadowHeight);
			glClear(GL_DEPTH_BUFFER_BIT);

			//not ideal to set uniforms not present, but keeps code simple for this demonstration
			glm::mat4 placeholder(1.0f);
			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				//will set model matrix for cube and assigns non-existent projection/view uniforms a placeholder value.
				RenderCube(cubeTransforms[i], shadowMapShader, vao, placeholder, placeholder);
			}

			//restore viewport for rendering
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, width, height);
			//--------------------------------------------------------------------------------------------


			//draw objects
			//tweak parameters
			shader.use();
			shader.setUniform1i("material.shininess", shininess);
			shader.setUniform3f("light.ambientIntensity", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("light.diffuseIntensity", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("light.specularIntensity", lightcolor.x, lightcolor.y, lightcolor.z);
			shader.setUniform1f("light.constant", 1);
			shader.setUniform1f("light.linear", 0.10f);
			shader.setUniform1f("light.quadratic", 0.065f);

			shader.setUniform1i("enableAmbient", toggleAmbient);
			shader.setUniform1i("enableDiffuse", toggleDiffuse);
			shader.setUniform1i("enableSpecular", toggleSpecular);
			shader.setUniform1i("bDisableAttenuation", bDisableAttenuation);
			shader.setUniform3f("light.position", lightPos.x, lightPos.y, lightPos.z);

			//--------------------------------------------------------------------------------------------
			shader.setUniform1f("lightFarPlaneDepth", lightFarPlaneDistance);
			shader.setUniform1i("lightDepthMap", 5);

			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubeMap);
			
			//--------------------------------------------------------------------------------------------

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shader, vao, projection, view);
			}
			shader.setUniform1i("bInvertNormals", 1);
			RenderCube(BoundingBox, shader, vao, projection, view);
			shader.setUniform1i("bInvertNormals", 0);

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
