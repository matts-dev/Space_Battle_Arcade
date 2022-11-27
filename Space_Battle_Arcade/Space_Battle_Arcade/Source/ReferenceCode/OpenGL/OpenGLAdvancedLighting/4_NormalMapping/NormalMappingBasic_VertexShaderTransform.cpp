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
				layout (location = 3) in vec3 tangent;	
				layout (location = 4) in vec3 bitangent;	
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				uniform vec3 cameraPosition;
				uniform vec3 lightPosition;

				out vec2 interpTextCoords;
				out vec3 fragNormal_TS;
				out vec3 fragPosition_TS;
				out vec3 lightPos_TS; //light position transformed to tangent space
				out vec3 cameraPosition_TS;
				out mat3 TBN_INV; //tangent_bitangent_normal
				

				void main(){
					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					mat3 normalModelMatrix = mat3(transpose(inverse(model)));

					fragNormal_TS = normalize(normalModelMatrix * normal);
					vec3 T = normalize(normalModelMatrix  * tangent);
					vec3 B = normalize(normalModelMatrix  * bitangent);

					TBN_INV = transpose(mat3(T, B, fragNormal_TS));

					//calculate lighting required vectors in tangent space
					//this is less expensive than doing matrix multiplies in a fragment shader, since the vertex shader has 3 runs per triangle, while the fragment shader will have a variable large amount of runs for a single triangle
					lightPos_TS = TBN_INV * lightPosition;
					cameraPosition_TS = TBN_INV * cameraPosition;
					fragPosition_TS = TBN_INV * vec3(model * vec4(position, 1));

					interpTextCoords = textureCoordinates;
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D diffuseMap;   
					sampler2D specularMap;
					sampler2D normalMap;
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

//--------------------------------------------------------------------------------------------
				//uniform vec3 cameraPosition;
				in vec3 cameraPosition_TS;
				in vec3 fragNormal_TS;
				in vec3 fragPosition_TS;
				in vec2 interpTextCoords;
				in mat3 TBN_INV ; //inverse tangent_bitangent_normal
				in vec3 lightPos_TS; //light position transformed to tangent space
//--------------------------------------------------------------------------------------------

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;
				uniform bool bInvertNormals = false;
				uniform bool bDisableAttenuation = false;
				uniform bool bUseNormalMapping = true;

				void main(){
	//--------------------------------------------------------------------------------------------
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.diffuseMap, interpTextCoords));	//diffuse color is generally similar to ambient color
					vec3 toLight = normalize(lightPos_TS - fragPosition_TS);

					vec3 normal;					
					if(bUseNormalMapping)
					{
						normal = texture(material.normalMap, interpTextCoords).rgb;
						normal = normalize(2.0f * normal - 1.0f);
					} 
					else 
					{
						normal = normalize(fragNormal_TS);			
					}
	
	//--------------------------------------------------------------------------------------------

					normal = bInvertNormals ? -normal : normal;

					vec3 diffuseLight = max(dot(toLight, normal), 0) * light.diffuseIntensity * vec3(texture(material.diffuseMap, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												//reflect expects vector from light position (tutorial didn't normalize this vector)
					vec3 toView = normalize(cameraPosition_TS - fragPosition_TS);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.specularMap, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(lightPos_TS - fragPosition_TS);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
					attenuation = bDisableAttenuation ? 1 : attenuation;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);

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
	bool bUseNormalMapping = true;
	//----------------------------------

	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press F to switch to depth framebuffer" << std::endl
				<< "Press R to toggle light rotation" << std::endl
				<< "Press T to enable light falloff attenuation" << std::endl
				<< "Press N to toggle normal mapping mode" << std::endl
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
		if (input.isKeyJustPressed(window, GLFW_KEY_N))
		{
			bUseNormalMapping = !bUseNormalMapping;
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

		//--------------------------------------------------------------------------------------------
		constexpr int numFloatsInVertex = 8;
		constexpr int numVertices = sizeof(vertices) / (numFloatsInVertex * sizeof(float));

		float tangents[numVertices * 3];
		float bitangents[sizeof(tangents)];

		for (int vertex = 0; vertex < numVertices; vertex += 3)
		{
			glm::vec3 v0 = glm::vec3(vertices[vertex * 8 + 0], vertices[vertex * 8 + 1], vertices[vertex * 8 + 2]);
			glm::vec3 v1 = glm::vec3(vertices[(vertex + 1) * 8 + 0], vertices[(vertex + 1) * 8 + 1], vertices[(vertex + 1) * 8 + 2]);
			glm::vec3 v2 = glm::vec3(vertices[(vertex + 2) * 8 + 0], vertices[(vertex + 2) * 8 + 1], vertices[(vertex + 2) * 8 + 2]);

			const uint32_t st_offset = 6;
			glm::vec2 st0 = glm::vec2(vertices[vertex * 8 + st_offset + 0], vertices[vertex * 8 + st_offset + 1]);
			glm::vec2 st1 = glm::vec2(vertices[(vertex + 1)* 8 + st_offset + 0], vertices[(vertex + 1) * 8 + st_offset + 1]);
			glm::vec2 st2 = glm::vec2(vertices[(vertex + 2) * 8 + st_offset + 0], vertices[(vertex + 2) * 8 + st_offset + 1]);

			glm::vec3 edge1_xyz = v1 - v0;
			glm::vec3 edge2_xyz = v2 - v0;

			glm::vec2 edge1_st = st1 - st0;
			glm::vec2 edge2_st = st2 - st0;

			//calculate bitangent and tangenet
			// notation: E = edge,  dU = delta U texture coordinate, dV = delta V texture coordinate, T = tangent, B = bitangent
			// note: in terms of texture st, U is s, V is t.
			//
			//E1 = dU1 * T + dV1 * B
			//E2 = dU2 * T + dV2 * B
			//
			//expressed in matrices
			//[ E1x E1y E1z ]  = [ dU1 dV1 ] * [ Tx Ty Tz ]
			//[ E2x E2y E2z ]  = [ dU2 dV2 ] * [ Bx By Bz ]
			//
			//*********** solved for B and T ****************
			//[ dU1 dV1 ]-1 * [ E1x E1y E1z ]  = [ Tx Ty Tz ]
			//[ dU2 dV2 ]   * [ E2x E2y E2z ]  = [ Bx By Bz ]
			//************************************************
			// express inverse calculation
			// (1 / (dU1*dV2 - dU2*dV1)) * [  dV2  -dV1 ] * [ E1x E1y E1z ]  = [ Tx Ty Tz ]
			//                             [ -dU2   dU1 ] *	[ E2x E2y E2z ]  = [ Bx By Bz ]
			float determinant = 1 / ((edge1_st.x * edge2_st.y) - (edge2_st.x * edge1_st.y));
			glm::mat2 adjugate(1.0f);
			//more efficient way to calculate this is to defer multiplication of determinant until later (ant not use matrix at all actually), but for clarity doing it this way
			adjugate[0][0] = determinant * edge2_st.y;  //column0
			adjugate[0][1] = determinant * -edge2_st.x; //column0
			adjugate[1][0] = determinant * -edge1_st.y; //column1
			adjugate[1][1] = determinant * edge1_st.x;  //column1

			//debug verify that glm is column major 
			//glm::vec2 col1 = adjugate[0] / determinant;
			//glm::vec2 col2 = adjugate[1] / determinant;

			glm::mat2& inverseMatrix = adjugate;

			glm::vec3 tangent;
			tangent.x = inverseMatrix[0][0] * edge1_xyz.x + inverseMatrix[1][0] * edge2_xyz.x;
			tangent.y = inverseMatrix[0][0] * edge1_xyz.y + inverseMatrix[1][0] * edge2_xyz.y;
			tangent.z = inverseMatrix[0][0] * edge1_xyz.z + inverseMatrix[1][0] * edge2_xyz.z;
			tangent = glm::normalize(tangent);

			glm::vec3 bitangent;
			bitangent.x = inverseMatrix[0][1] * edge1_xyz.x + inverseMatrix[1][1] * edge2_xyz.x;
			bitangent.y = inverseMatrix[0][1] * edge1_xyz.y + inverseMatrix[1][1] * edge2_xyz.y;
			bitangent.z = inverseMatrix[0][1] * edge1_xyz.z + inverseMatrix[1][1] * edge2_xyz.z;
			bitangent = glm::normalize(bitangent);

			//store calculate mesh tangents/bitangents
			for (int adjacent = 0; adjacent < 3; ++adjacent)
			{
				tangents[vertex * 3 + adjacent * 3 + 0] = tangent.x;
				tangents[vertex * 3 + adjacent * 3 + 1] = tangent.y;
				tangents[vertex * 3 + adjacent * 3 + 2] = tangent.z;

				bitangents[vertex * 3 + adjacent * 3 + 0] = bitangent.x;
				bitangents[vertex * 3 + adjacent * 3 + 1] = bitangent.y;
				bitangents[vertex * 3 + adjacent * 3 + 2] = bitangent.z;
			}
		}

		//--------------------------------------------------------------------------------------------
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

		//--------------------------------------------------------------------------------------------
		GLuint tangentsVBO;
		glGenBuffers(1, &tangentsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tangentsVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tangents), tangents, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(3);

		GLuint bitangentsVBO;
		glGenBuffers(1, &bitangentsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, bitangentsVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bitangents), bitangents, GL_STATIC_DRAW);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(4);
		
		//--------------------------------------------------------------------------------------------

		glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.

		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//textures
		GLuint diffuseMap = textureLoader("Textures/brickwall.jpg", GL_TEXTURE0);
		GLuint specularMap = textureLoader("Textures/brickwall.jpg", GL_TEXTURE1);
		//--------------------------------------------------------------------------------------------
		stbi_set_flip_vertically_on_load(false);
		GLuint normalMap = textureLoader("Textures/brickwall_normal.jpg", GL_TEXTURE2);
		//--------------------------------------------------------------------------------------------

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.diffuseMap", 0);
		shader.setUniform1i("material.specularMap", 1);
		//--------------------------------------------------------------------------------------------
		shader.setUniform1i("material.normalMap", 2);
		//--------------------------------------------------------------------------------------------

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(0.0f, 0.0f, 2.0f);
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
			//{ { 0.0f, -0.0f, 0.0f },{ 0, 0, 0 },{ 2.0f,2.0f,0.1f } },
			{ { 0.0f, -0.0f, 0.0f },{ 0, 0, 0 },{ 1.0f,1.0f,1.0f } },
			{ { -1.5f, 2.0f, -3.0f },{ 60, 0, 60 },{ 0.75f,0.75f,0.75f } }
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
			shader.setUniform3f("light.position", lightPos.x, lightPos.y, lightPos.z);
			//--------------------------------------------------------------------------------------------
			//send copy of light position to vert shader, rather than duplicating light struct in vert shader
			shader.setUniform3f("lightPosition", lightPos);
			//--------------------------------------------------------------------------------------------
			shader.setUniform1i("bDisableAttenuation", bDisableAttenuation);
			shader.setUniform1i("bUseNormalMapping", bUseNormalMapping);

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shader, vao, projection, view);
			}

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &tangentsVBO);
		glDeleteBuffers(1, &bitangentsVBO);

		glDeleteVertexArrays(1, &lampVAO);
	}
}

//int main()
//{
//	true_main();
//}
