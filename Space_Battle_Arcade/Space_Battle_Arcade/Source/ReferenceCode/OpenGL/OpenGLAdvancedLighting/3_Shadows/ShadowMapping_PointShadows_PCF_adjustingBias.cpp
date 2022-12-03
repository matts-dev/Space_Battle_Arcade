
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

//--------------------------------------------------------------------------------
				uniform samplerCube depthMap;
				uniform float lightFarPlane;
				float depthDebugVisualizer;

				//separable directions for efficient PCF
				vec3 sampleOffsetDirections[20] = vec3[]
				(
				   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
				   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
				   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
				   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
				   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
				); 

				float isInShadow(){
					vec3 lightToFrag_ws = fragPosition - light.position;
					float toFragDepth = length(lightToFrag_ws);
					
					//NO PCF
					//float closestDepthVisibleFromLight = texture(depthMap, lightToFrag_ws).r;
					//closestDepthVisibleFromLight *= lightFarPlane; //transform from [0, 1] to [0, lightFarPlane]
					//depthDebugVisualizer = closestDepthVisibleFromLight;
					//float bias = 0.05f;
					//float shadow = (toFragDepth - bias) > closestDepthVisibleFromLight ? 1.0f : 0.0f;
					//return shadow;

					//SIMPLE PCF (extremely expensive version)
					//float bias = 0.05f;
					//float shadow = 0.0f;
					//float samples = 4;
					//float offset = 0.1f; //should use texture size here? perhaps issues with cube maps. Looks like textureSize for samplerCube was supported in OpenGL4.00
					//for(float x = -offset; x < offset; x += offset / (samples * 0.5f)) {
					//	for(float y = -offset; y < offset; y += offset / (samples * 0.5f)) {
					//		for(float z = -offset; z < offset; z += offset / (samples * 0.5f)) {
					//			float closestDepthVisibleFromLight = texture(depthMap, lightToFrag_ws + vec3(x, y, z)).r;
					//			closestDepthVisibleFromLight *= lightFarPlane;
					//			shadow += (toFragDepth - bias) > closestDepthVisibleFromLight ? 1.0f : 0.0f;
					//		}
					//	}
					//}
					//return shadow / (samples * samples * samples);

					//EFFICIENT PCF
					int samples = 20;
					float shadow = 0.0f;
					//float diskRadius = 0.15f;
					float diskRadius = (1.0 + toFragDepth/lightFarPlane) / lightFarPlane; //!!!!!!!!should cap this at some fraction of bias for clean results
					float bias = 0.05;
					//float bias = max(0.005, (1.0f - dot(normalize(-lightToFrag_ws), normalize(fragNormal))) * 0.05); //this could be optimized by just passing these calculated normalized vecs from caller

					for(int i = 0; i < samples; ++i){
						float closestDepthVisibleFromLight  = texture(depthMap, lightToFrag_ws + sampleOffsetDirections[i] * diskRadius).r;
						closestDepthVisibleFromLight *= lightFarPlane; //[0, 1] -> [0, farPlane]
						shadow += (toFragDepth - bias) > closestDepthVisibleFromLight ? 1.0f : 0.0f;
					}
					shadow /= samples;

					//optionally prevent shadow from being outside of far plane... this isn't too realistic though (as it was for directional)
					//shadow = toFragDepth > lightFarPlane ? 0.0f : shadow;

					return shadow;
				}

//------------------------------------------------------------------
				

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

//----------------------------------------------------------
					float isLit = (1.0f - isInShadow());
					vec3 lightContribution = (ambientLight + isLit * (diffuseLight + specularLight)) * vec3(attenuation);
					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation
//----------------------------------------------------------

					fragmentColor = vec4(lightContribution, 1.0f);
//----------------------------------------------------------------------
					//debug visualize of depth map
					//fragmentColor = vec4(vec3(depthDebugVisualizer / lightFarPlane), 1.0f);
//----------------------------------------------------------------------
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
	const char* depth_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;

				void main(){
					gl_Position = model * vec4(position, 1);
				}
			)";
	const char* depth_geometry_shader_src = R"(
				#version 330 core
				
				layout (triangles) in; 	
				layout (triangle_strip, max_vertices=18) out;		//18 because 3 vertices are transformed for the 6 faces, thus 3 * 6 = 18

				//best to do the combination of matrices (ie proj * view) on cpu side
				uniform mat4 shadowTransforms[6];

				//fragment's position in world space
				out vec4 fragPos_ws;

				void main(){
					for(int face = 0; face < 6; ++face){
						gl_Layer = face;		//gl_Layer is a builtin that specifies face in cubemap

						for(int vertex = 0; vertex < 3; ++vertex){
							//I believe gl_Position this will have perspective division applied to it, so we need to pass a variable to the fragment shader as is to manually work with depth
							//reminder, these values (not gl_Position) will be interpolated like normal variables passed from vertex to fragment shader; (eg textureCoordinates)
							fragPos_ws = gl_in[vertex].gl_Position;

							//transform actual rasterized point to clip space
							gl_Position = shadowTransforms[face] * fragPos_ws; 

							//debug cubemap generation; each face contents should have a different scale of gray.
							//fragPos_ws = vec4(vec3( (face + 1) / 7 ), 1.0f); 

							EmitVertex();
						}
						EndPrimitive();
					}		
				}
			)";
	const char* depth_frag_shader_src = R"(
				#version 330 core

				in vec4 fragPos_ws;

				uniform vec3 lightPosition_ws; //ws = world space
				uniform float farPlane;
				
				void main(){
					vec3 fragToLight = lightPosition_ws - fragPos_ws.xyz;
					float depthToLight = length(fragToLight);								
					
					//transform depth to range [0, 1]
					float depthInRange = depthToLight / farPlane;
					
					//update built-in so that this value is written to the depth buffer, rather than the non-linear depth from the projection matrix
					gl_FragDepth = depthInRange;
				}
			)";

	const char* depthcubemapVisualizer_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				//viewspace frag position
				out vec3 textureCoord;

				void main(){
					textureCoord = vec3(model * vec4(position, 1));

					//ignore view translation by chopping it out of the matrix
					vec4 fragPosition_vs =  model * vec4(position, 1);
					fragPosition_vs = vec4(mat3(view) * vec3(fragPosition_vs), 1);

					gl_Position = projection * fragPosition_vs;
				}
			)";
	const char* depthcubemapVisualizer_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				
				in vec3 textureCoord;
				uniform samplerCube depthMap;
				
				void main(){
					float depth = texture(depthMap, textureCoord).r;
					
					fragmentColor = vec4(vec3(depth), 1.0f);
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
	bool bShowDepthMap = false;
	float farDistanceIncrement = 1.0f;
	float lightFarPlaneManipulatiableValue = 25.0f;
	//----------------------------------

	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press F to switch to depth framebuffer" << std::endl
				<< "Press R to toggle light rotation" << std::endl
				<< "Press T to enable light falloff attenuation" << std::endl
				<< "Press Up/Down to increment the far plane distance" << std::endl
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
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			bShowDepthMap = !bShowDepthMap;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_UP))
		{
			lightFarPlaneManipulatiableValue += farDistanceIncrement;
			std::cout << lightFarPlaneManipulatiableValue << std::endl;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_DOWN))
		{
			lightFarPlaneManipulatiableValue -= farDistanceIncrement;
			std::cout << lightFarPlaneManipulatiableValue << std::endl;
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
		GLuint specularMap = textureLoader("Textures/woodfloor.png", GL_TEXTURE1);

		//--------------------------------- CREATE DEPTH CUBEMAP ------------------------------------
		GLuint depthCubeMapTex;
		glGenTextures(1, &depthCubeMapTex);

		const unsigned int shadowPXWidth = 1024;
		const unsigned int shadowPXHeight = 1024;
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapTex);

		for (int i = 0; i < 6; ++i)
		{
			//incrementing to GL_TEXTURE_CUBE_MAP_POSITIVE_X allows us ot specify each face incrementally.
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, shadowPXWidth, shadowPXHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		//--------------------------------------------------------------------------------------------

		//shaders
		Shader shader(vertex_shader_src, frag_shader_src, false);
		//activate texture units
		shader.use();
		shader.setUniform1i("material.diffuseMap", 0);
		shader.setUniform1i("material.specularMap", 1);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);
		Shader shadowMappingShader(depth_vertex_shader_src, depth_frag_shader_src, depth_geometry_shader_src, false);
		Shader depthMapViewerShader(depthcubemapVisualizer_vertex_shader_src, depthcubemapVisualizer_frag_shader_src, false);

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


		//--------------------------------------------------------------------------------------------
		//Create a depth map framebuffer
		GLuint shadowDepthFBO;
		glGenFramebuffers(1, &shadowDepthFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowDepthFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMapTex, 0);
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "[failure] to set up depth framebuffer" << std::endl;
		}

		//bind back to the default frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//--------------------------------------------------------------------------------------------



		float lastLightRotationAngle = 0.0f;
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
			lastLightRotationAngle = rotateLight ? 100 * currentTime : lastLightRotationAngle;
			model = glm::rotate(model, glm::radians(lastLightRotationAngle), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
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
			//The same projection matrix is used for the 6 faces of the cubemap
			//we want a FOV of 90 degrees so that each face fully covers a face
			float lightAspect = ((float)shadowPXWidth) / shadowPXHeight;
			float lightNearPlane = 0.1f;//1.0f;
			float lightFarPlane = lightFarPlaneManipulatiableValue; // 25.0f;
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(90.0f), lightAspect, lightNearPlane, lightFarPlane);

			//prepare shadow map's view and projection matrices
			//note: the tutorial choses some strange up values here, I am trying out some more intuitive ones to avoid just using magic numbers
			std::vector<glm::mat4> viewMatrices;
			
			//The ordering below must match ordering of the GL_TEXTURE_CUBE_MAP_POSITIVE_X
			//This is the internal ordering of the cube map's faces, which is respected in the geometry shader too.
			//The up directions are tricky here, but can be figured out one face at a time.
			//tips for finding up values: 
			//	*the +x,-x,+z,-z faces will all have the same up direction
			//	*the axis pointing at will not be an up direction 
			//	*once the +x,-x,+z,-z are found, tilt your hand while pointing down the +z axis to find the top/bottom face's up value (it will be your thumb)
			//  *apply either an axis switch, or a negation of the current axis
			//  *negating the current axis SIGN gives a visual appearance of mirroring
			//  *changing the current axis gives visual appearance of rotating (and maybe mirroring too)

			//verified empirically. For the x and z faces, the up value is always down. To understand the y planes up values, 
			//use your hand for the ortho normal basis where your finger points in the x/z direction and thumb towards the 'up' direction, which is down.
			//if you tilt your finger to point up or down (around x-axis while pointing down +z), your thumb will point in the up direction for the up/down face.
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, -1, 0)));
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0, -1, 0)));
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 0, 1)));
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0, 0, -1)));
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0, -1, 0)));
			viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0, -1, 0)));

			//helpers for finding appropriate up directions. Whie the up values are (0,0,0), the face won't draw. 
			// to find the up directiosn empiraclly, work with once face at a time.
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, 0, 0)));
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0, 0, 0)));
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 0, 0)));
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0, 0, 0)));
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0, 0, 0)));
			//viewMatrices.emplace_back(glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0, 0, 0)));
		
			//--------------------------------------------------------------------------------------------

			//--------------------------------------------------------------------------------------------
			// DRAW TO DEPTH MAP
			glBindFramebuffer(GL_FRAMEBUFFER, shadowDepthFBO);
			glViewport(0, 0, shadowPXWidth, shadowPXHeight);
			glClear(GL_DEPTH_BUFFER_BIT);

			shadowMappingShader.use();
			for (size_t i = 0; i < viewMatrices.size(); ++i)
			{
				glm::mat4 lightTransformPV = depthProjectionMatrix * viewMatrices[i];
				std::string uniformName = "shadowTransforms[" + std::to_string(i) + "]";
				shadowMappingShader.setUniformMatrix4fv(uniformName.c_str(), 1, GL_FALSE, glm::value_ptr(lightTransformPV));
			}
			shadowMappingShader.setUniform1f("farPlane", lightFarPlane);
			shadowMappingShader.setUniform3f("lightPosition_ws", lightPos);

			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shadowMappingShader, vao, projection, view);
			}
			//RenderCube(BoundingBox, shadowMappingShader, vao, projection, view);

			//restore normal framebuffer and viewport for rendering
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
			shader.setUniform3f("light.position", lightPos.x, lightPos.y, lightPos.z);
			shader.setUniform1i("bDisableAttenuation", bDisableAttenuation);
			//--------------------------------------------------------------------------------------------
			shader.setUniform1f("lightFarPlane", lightFarPlane);

			//activate the depth map as the 5th texture slot
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapTex);
			shader.setUniform1i("depthMap", 5); 
			
			//--------------------------------------------------------------------------------------------

			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			for (size_t i = 0; i < sizeof(cubeTransforms) / sizeof(Transform); ++i)
			{
				RenderCube(cubeTransforms[i], shader, vao, projection, view);
			}
			shader.setUniform1i("bInvertNormals", 1);
			RenderCube(BoundingBox, shader, vao, projection, view);
			shader.setUniform1i("bInvertNormals", 0);

			
			if (bShowDepthMap)
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				depthMapViewerShader.use();
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapTex);
				depthMapViewerShader.setUniform1i("depthMap", 5);

				RenderCube(BoundingBox, depthMapViewerShader, vao, projection, view);
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
