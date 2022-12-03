
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

				out vec3 fragNormal_WS;
				out vec3 fragPosition_WS;
				out vec2 texCoords;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition_WS = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal_WS = mat3(transpose(inverse(model))) * normal;

					texCoords = inTexCoords;
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core

				out vec4 fragmentColor;

				uniform vec3 objectColor;
				uniform vec3 cameraPosition;

				#define NUM_LIGHTS 4
				uniform vec3 lightPosition[NUM_LIGHTS];
				uniform vec3 lightColor[NUM_LIGHTS];
				uniform int numLightsToRender = 4;

				in vec3 fragNormal_WS;
				in vec3 fragPosition_WS;
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
				uniform int screenCaptureIdx = 0;

				uniform vec3 albedo_u;
				uniform float metalic_u;
				uniform float roughness_u;
				uniform float AO_u = 0.0f;

				uniform bool bUseTextureData = false;
				
				const float PI = 3.141592;

                //________________DISTRIBUTION________________
				float D_ThrowbridgeReitzGGX(vec3 normal, vec3 halfway, float roughness){
					float r_squared = roughness * roughness;
					r_squared *= r_squared; //tutorial shows extra squaring, I think it refers to an approach by disney/epic
					float n_dot_h = max(dot(normal, halfway), 0.0);
					float n_dot_h_squared = n_dot_h * n_dot_h;

                    float denom_term1 = n_dot_h_squared * (r_squared - 1.0f) + 1.0f;
					float denom = (PI * denom_term1 * denom_term1);
					denom = max(denom, 0.000000001f);
					return r_squared / denom;
				}


                //________________GEOMETRY________________
                float geometryRoughnessMap_Direct(float roughness) { return ((roughness + 1)*(roughness + 1)) / 8; }
                float geometryRoughnessMap_IBL(float roughness) { return (roughness * roughness) / 2; }
                float G_SchlickBeckmannGGX(vec3 normal, vec3 direction, float mappedRoughness)
                {
                    float n_dot_d = dot(normal, direction);
					n_dot_d = max(n_dot_d, 0.0f);

                    float denominator = n_dot_d * (1.0f - mappedRoughness) + mappedRoughness;
                    return n_dot_d / denominator;
                }
                float G_Smiths(vec3 normal, vec3 toView, vec3 toLight, float mappedRoughness)
                {
                    return G_SchlickBeckmannGGX(normal, toView, mappedRoughness) * G_SchlickBeckmannGGX(normal, toLight, mappedRoughness);
                }


                //________________FRESNEL________________
                vec3 F_FreshnelApproximation(vec3 halfway, vec3 toView, vec3 zeroIncidenceConstant)
                {
					float cos_theta = max(dot(halfway, toView), 0.0f);

                    return zeroIncidenceConstant + (1.0f - zeroIncidenceConstant) * pow(1.0f - cos_theta, 5.0f);
                }

				//-------------- Normal Mapping Trick -------------
				//the precomputation of tangents and bitangents is preferable to this method for performance reasons
				//but it is a relatively simple peice of code to add for a quick demo
				vec3 getMappedNormal(vec3 vertNormal){
					//this is based on tutorial from learnopengl textured direct lights; method of precomputing TBN into vertex buffers should be preferred
					//helpful resources for understanding below
					//http://www.aclockworkberry.com/shader-derivative-functions/
					//http://hacksoflife.blogspot.com/2009/11/per-pixel-tangent-space-normal-mapping.html

					vec3 textureTangentNormal = texture(material.normal,texCoords).rgb; 
					textureTangentNormal = (textureTangentNormal * 2.0f) - 1.0f;

					vec3 dWorldSpace_dx = dFdx(fragPosition_WS);
					vec3 dWorldSpace_dy = dFdy(fragPosition_WS);

					vec2 dST_dx = dFdx(texCoords);
					vec2 dST_dy = dFdy(texCoords);

					vec3 N = normalize(vertNormal);
					vec3 T = normalize(dWorldSpace_dx * dST_dy.t - dWorldSpace_dy * dST_dx.t);
					vec3 B = -normalize(cross(N, T));
					mat3 TBN = mat3(T, B, N);

					return normalize(TBN * textureTangentNormal);
				}

				#define DIELECTRIC_ZERO_INCIDENCE 0.04

				void main(){

					vec3 capture1_D, capture2_G, capture3_F, capture4_r, capture5_m, capture6_Gl, capture7_Gv, capture8_albedo, capture9_normal;

					vec3 albedo;
					float roughness;
					float metalness;
					float AO;
					vec3 normal;
					vec4 capture;
					
					if(bUseTextureData){
						albedo = texture(material.albedo,texCoords).rgb;
						//normal = texture(material.normal,texCoords).rgb; 
						//normal = normalize(fragNormal_WS); 
						normal = getMappedNormal(fragNormal_WS);
						metalness = texture(material.metalic,texCoords).r;
						roughness = texture(material.roughness,texCoords).r;
						AO = texture(material.AO,texCoords).r;
					}
					else {
						albedo = albedo_u;
						metalness = metalic_u;
						roughness = roughness_u;
						AO = AO_u;
						normal = normalize(fragNormal_WS); 
					}

					vec3 L0 = vec3(0,0,0);					
					vec3 toView = normalize(cameraPosition - fragPosition_WS);

					vec3 zeroIncidence = mix(vec3(DIELECTRIC_ZERO_INCIDENCE), albedo.rgb, metalness);

					for(int light_idx = 0; light_idx < NUM_LIGHTS && light_idx < numLightsToRender; ++light_idx){
						vec3 toLight = normalize(lightPosition[light_idx] - fragPosition_WS);
						vec3 halfVec = normalize(toView + toLight);

						float lightDistance = length(lightPosition[light_idx] - fragPosition_WS);
						float invSqrAttenuation = 1.0f / (lightDistance * lightDistance);
						vec3 lightColor_i = lightColor[light_idx] * invSqrAttenuation;

						//cook-torrence3
						float D = D_ThrowbridgeReitzGGX(normal, halfVec, roughness);
						float G = G_Smiths(normal, toView, toLight, geometryRoughnessMap_Direct(roughness));
						vec3  F = F_FreshnelApproximation(halfVec, toView, zeroIncidence);

						//F is effectively kSpecular 
						vec3 kDiffuse = vec3(1.0f) - F;		
						kDiffuse = kDiffuse * (1.0f - metalness);		//metals do not have diffuse
						
						float viewDotNorm = max(dot(toView, normal), 0.0f);
						float lightDotNorm = max(dot(toLight, normal), 0.0f);
						float microFacetNormalization = 4 * viewDotNorm * lightDotNorm;
						vec3 lRefract = kDiffuse * (albedo / PI);
						vec3 lReflect = (D * G * F) / max(microFacetNormalization, 0.001f);
						L0 += (lRefract + lReflect) * lightColor_i * max(dot(toLight, normal),0.0f);

								//capture visualizes (mostly for debugging and demonstration purposes, not part of real PBR shader)
								//deciding not to do these are arrays so they're easy to identify what is being captured
								capture = vec4(normal, 1.0f);
								capture1_D = vec3(D);
								capture2_G = vec3(G);
								capture3_F = F;
								capture4_r = vec3(roughness); //these don't have to be in loop, but keeping them in same spot as other since this is a debug feature
								capture5_m = vec3(metalness);
								capture6_Gl = vec3(G_SchlickBeckmannGGX(normal, toLight, geometryRoughnessMap_Direct(roughness)));
								capture7_Gv = vec3(G_SchlickBeckmannGGX(normal, toView, geometryRoughnessMap_Direct(roughness)));
								capture8_albedo = albedo;
								capture9_normal = normal;
					}							

					//adhoc ambiance; IBL will provide ambience when implemented in a later file
					L0 += vec3(0.03f) * albedo * (AO);

					//map to ldr (Reinhard method)
					L0 = L0 / (L0 + 1);
				
					//gamma correct
					const float GAMMA = 2.2f;
					L0 = pow(L0, vec3(1.0f/GAMMA));

					fragmentColor = vec4(L0, 1.0f);

							//DEBUG
							if(screenCaptureIdx == 0){
								//do nothing, fragment is the result of PBR
							} else if (screenCaptureIdx == 1){
								fragmentColor = vec4(capture1_D, 1.0f);
							}else if (screenCaptureIdx == 2){
								fragmentColor = vec4(capture2_G, 1.0f);
							}else if (screenCaptureIdx == 3){
								fragmentColor = vec4(capture3_F, 1.0f);
							}else if (screenCaptureIdx == 4){
								fragmentColor = vec4(capture4_r, 1.0f);
							}else if (screenCaptureIdx == 5){
								fragmentColor = vec4(capture5_m, 1.0f);
							}else if (screenCaptureIdx == 6){
								fragmentColor = vec4(capture6_Gl, 1.0f);
							}else if (screenCaptureIdx == 7){
								fragmentColor = vec4(capture7_Gv, 1.0f);
							}else if (screenCaptureIdx == 8){
								fragmentColor = vec4(capture8_albedo, 1.0f);
							}else if (screenCaptureIdx == 9){
								fragmentColor = vec4(capture9_normal, 1.0f);
							}
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


	template <typename T>
	T clamp(T value, T min, T max)
	{
		value = value < min ? min : value;
		value = value > max ? max : value;
		return value;
	}

	bool rotateLight = true;
	bool bUseWireFrame = false;
	bool bUseTexture = false;
	unsigned int numLightsToRender = 4;
	int displayScreen = 0;
	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press R to toggle light rotation" << std::endl
				<< " Up/Down to change number of lights rendered " << std::endl
				<< " Press T to toggle between textured mode" << std::endl
				<< " Press number keys to change views" << std::endl
				<< " \t 0=PBR, 1=D, 2=G, 3=F, 4=rough, 5=metal, 6=geoLightComponent, 7=geoViewComponent, 8=albedo, 9=normal" << std::endl
				//vec3 capture1_D, capture2_G, capture3_F, capture4_r, capture5_m, capture6_Gl, capture7_Gv, capture8_albedo, capture9_normal;
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
		unsigned int minLightNum = 1, maxLightNum = 4;
		if (input.isKeyJustPressed(window, GLFW_KEY_UP))
		{
			numLightsToRender = clamp(numLightsToRender + 1, minLightNum, maxLightNum);
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_DOWN))
		{
			numLightsToRender = clamp(numLightsToRender - 1, minLightNum, maxLightNum);
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_T))
		{
			bUseTexture = !bUseTexture;
		}

		for (int key = GLFW_KEY_0; key < GLFW_KEY_9 + 1; ++key)
		{
			if (input.isKeyJustPressed(window, key))
			{
				displayScreen = key - GLFW_KEY_0;
			}
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
		camera.setSpeed(5.0f);
		int width = 1200;
		int height = 800;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		bool useSRGB = false;
		GLuint albedoTexure = textureLoader("Textures/pbr_rustediron1/rustediron2_basecolor.png", GL_TEXTURE0, true);
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

		//HDR light values; at least tutorial used 300 so inverse square falloff would look realistic
		glm::vec3 lightcolor(300.0f, 300.0f, 300.0f);
		glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);

		glm::vec3 objectPos;
		glm::vec3 lightPos(1.2f, 5.0f, 2.0f);
		std::vector<glm::vec3> lights = {
			glm::vec3(1.2f, 5.0f, 2.0f),
			glm::vec3(-10, -10, 2.0f),
			glm::vec3(-10,  10, 2.0f),
			glm::vec3( 10,  10, 2.0f)
		};

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

				float roughness = col / static_cast<float>(numCols);
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

			lightcolor = glm::vec3(bUseTexture ? 150.0f : 300.0f);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);
			glm::mat4 model;

			//draw light mesh
			lampShader.use();
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			//render light meshes
			for (unsigned int light_idx = 0; light_idx < lights.size() && light_idx < (numLightsToRender); ++light_idx)
			{
				lastLightRotationAngle = rotateLight && light_idx == 0? 100 * currentTime : lastLightRotationAngle;
				model = glm::mat4(1.0f);
				model = glm::rotate(model, glm::radians(light_idx == 0 ? lastLightRotationAngle : 0.0f), glm::vec3(0, 0, 1)); //apply rotation leftmost (after translation) to give it an orbit
				model = glm::translate(model, lights[light_idx]);
				model = glm::scale(model, glm::vec3(0.2f));
				lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				sphereMesh.render();
			}

			//draw objects
			shader.use();
			shader.setUniform1i("numLightsToRender", numLightsToRender);
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniform3f("objectColor", objectcolor.x, objectcolor.y, objectcolor.z);
			const glm::vec3& camPos = camera.getPosition();
			shader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			shader.setUniform1i("bUseTextureData", bUseTexture);
			shader.setUniform1i("screenCaptureIdx", displayScreen);


			//send lighting data to shader
			for (unsigned int i = 0; i < lights.size(); ++i)
			{
				std::string prefixColor = "lightColor[";
				std::string suffix = "]";
				std::string uniformColor = prefixColor + std::to_string(i) + suffix;
				shader.setUniform3f(uniformColor.c_str(), lightcolor.x, lightcolor.y, lightcolor.z);

				std::string prefixPosition= "lightPosition[";
				std::string uniformPosition = prefixPosition + std::to_string(i) + suffix;
				glm::vec3 lightPos = lights[i];
				if (i == 0) //allow rotation of first light
				{ 
					model = glm::mat4(1.0f);
					model = glm::rotate(model, glm::radians(lastLightRotationAngle), glm::vec3(0, 0, 1)); //apply rotation leftmost (after translation) to give it an orbit
					lightPos = glm::vec3(model * glm::vec4(lightPos, 1.0f));
				}
				shader.setUniform3f(uniformPosition.c_str(), lightPos.x, lightPos.y, lightPos.z);
			}

			for (unsigned int idx = 0; idx < sphereMeshes.size(); ++idx)
			{
				model = glm::mat4(1.0f);
				model = glm::translate(model, sphereMeshes[idx].position);
				shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader.setUniform1f("metalic_u", sphereMeshes[idx].metalness);
				shader.setUniform1f("roughness_u", sphereMeshes[idx].roughness);
				shader.setUniform3f("albedo_u", sphereMeshes[idx].albedo);
				shader.setUniform1f("AO_u", sphereMeshes[idx].ao);
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