#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../nu_utils.h"
#include "../../Shader.h"
#include "../../Libraries/stb_image.h"
#include "../GettingStarted/Camera/CameraFPS.h"
#include "../Utilities/SphereMesh.h"
#include "../../InputTracker.h"
#include "../Utilities/SphereMeshTextured.h"
#include "../Utilities/SimpleCubeMesh.h"

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

				uniform samplerCube irradianceMap;

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
				/** This version is for ambient light where there are no roughness terms in the calculation */
                vec3 F_FreshnelApproximationRoughnessVersion(vec3 halfway, vec3 toView, vec3 zeroIncidenceConstant, float roughness)
                {
					float cos_theta = max(dot(halfway, toView), 0.0f);
                    return zeroIncidenceConstant + ( max(vec3(1.0f - roughness) - zeroIncidenceConstant, 0.0f) ) * pow(1.0f - cos_theta, 5.0f);
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

					// IBL based diffuse ambient
					vec3 ambient = texture(irradianceMap, normal).rgb;
						//vec3 kSpecular = F_FreshnelApproximation(normal, toView, zeroIncidence);
					vec3 kSpecular = F_FreshnelApproximationRoughnessVersion(normal, toView, zeroIncidence, roughness);
					vec3 kDiffuse = vec3(1.0f) - kSpecular;
					kDiffuse *= (1.0f - metalness); //metalness should always be on range [0, 1]
					L0 += ambient * albedo * kDiffuse * AO;

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
///-------------------------------------------------------------------------------------------
	const char* equiToCube_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				

				out vec3 localPos;
				
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					//we do not want texture rotating with the cubemap, so this lacks view transform
					localPos = position;

					//rotate cube map appropriately
					gl_Position = projection * view * vec4(position, 1);
				}
			)";
	const char* equiToCube_frag_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec3 localPos;

				uniform sampler2D equirectangularHDRImage;
							
				const vec2 invAtan = vec2(0.1591f, 0.3183f);
				void main(){
					vec3 dir = normalize(localPos);			
					
					//tutorial doesn't cover the math behind going from a equirectangular projection back to sphere.
					//sample sphere position in equirect map
					vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
					uv *= invAtan;
					uv += 0.5f;

					//sample equirectangular map with converted coordinates
					vec3 color = texture(equirectangularHDRImage, uv).rgb;

					//debug
					//color = vec3(1,0,0);
					//color = texture(equirectangularHDRImage, -vec2(0.4f, 0.5f)).rgb;
					color = color / (1 + color);
					
					fragmentColor = vec4(color, 1.0f);
				}
			)";
	///-------------------------------------------------------------------------------------------
	const char* skybox_vert_shader = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				

				out vec3 texCoord;

				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					mat4 view_notranslation = mat4(mat3(view));

					texCoord = position;

					gl_Position = projection * view_notranslation * vec4(position, 1);
					gl_Position = gl_Position.xyww; //force depth to be largest possible
				}
			)";
	const char* skybox_frag_shader = R"(
				#version 330 core
				out vec4 fragmentColor;
				
				in vec3 texCoord;

				uniform samplerCube skybox;
				
				void main(){
					//in order for this to pass the depth test, the depth function must be set to glDepthFunc(GL_LEQUAL) otherwise this will never be less than default depth
					vec3 color = texture(skybox, texCoord).rgb;
					
					//HDR tonemap to LDR
					color = color / (1 + color);

					//gamma correct
					color = pow(color, vec3(1.0f/2.2f));

					fragmentColor = vec4(color, 1.0f);
				}
			)";
///-------------------------------------------------------------------------------------------
	const char* hemisphere_convolver_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 localPosition;

				void main(){
					
					mat4 rotatedView = mat4(mat3(view)); //trim off translation

					localPosition = position;
					gl_Position = projection * rotatedView * vec4(position, 1);
				}
			)";
	const char* hemisphere_convolver_frag_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec3 localPosition;

				uniform samplerCube environmentMap;

				const float PI = 3.14159;
				
				void main(){
					//create basis vectors for transforming local(tangent) space into world space.
					vec3 normal = normalize(localPosition);
					vec3 tempUp = vec3(0.0f,1.0f,0.0f);
					vec3 right = normalize(cross(tempUp, normal));
					vec3 up = normalize(cross(normal, right));

					float stepSize = 0.025f;
					float horiStep = stepSize;// * (2 * PI);
					float vertStep = stepSize;// * PI/2;

					vec3 irradiance = vec3(0.0f);			
					int numSamples = 0;

					for(float hori_angle = 0.0f; hori_angle < 2.0f * PI; hori_angle += horiStep)
					{
						for(float angle_from_zenith = 0.0f; angle_from_zenith < PI * 0.5f; angle_from_zenith += vertStep)
						{
							vec3 localPos;
							localPos.x = sin(angle_from_zenith) * cos(hori_angle);
							localPos.y = sin(angle_from_zenith) * sin(hori_angle);
							localPos.z = cos(angle_from_zenith);

							vec3 worldPos = (right * localPos.x) + (up * localPos.y) + (normal * localPos.z);
							
							//cos(angle_from_zenith) accounts for incoming light angle reducing brightness (same as in blinn-phong)
							//sin(angle_from_zenith) weights samples near horrizontal more heavily to compensate for more samples around zenith (remember: sin(90) = 1, sin(0) = 0; 0 being at zenth, and 90 being at horrizontal)
							irradiance += texture(environmentMap, worldPos).rgb * cos(angle_from_zenith) * sin(angle_from_zenith);
							numSamples++;
						}
					}

					//average irradiance, pre-multiply by pi so when we divide by pi later it cancels out.
					irradiance = (irradiance * PI) / float(numSamples);

					fragmentColor = vec4(irradiance, 1.0f);
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
	int activeEnvironmentMap = 0;
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

	GLuint loadHDRTexture(const char* filepath)
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, numChannels;
		float* data = stbi_loadf(filepath, &width, &height, &numChannels, 0);

		GLuint hdrTexture = 0;
		if (data)
		{
			glGenTextures(1, &hdrTexture);
			glBindTexture(GL_TEXTURE_2D, hdrTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			stbi_image_free(data);
		}
		else
		{
			std::cerr << "ERROR LOADING IMAGE: " << filepath << std::endl;
			std::exit(-1);
		}

		return hdrTexture;
	}

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

		//HDR environment equirectangular textures
		GLuint hdr_newportTexture = loadHDRTexture("Textures/hdr/newport_loft.hdr");


		Shader shader(vertex_shader_src, frag_shader_src, false);
		shader.use();
		shader.setUniform1i("material.albedo", 0);
		shader.setUniform1i("material.normal", 1);
		shader.setUniform1i("material.metalic", 2);
		shader.setUniform1i("material.roughness", 3);
		shader.setUniform1i("material.AO", 4);
		shader.setUniform1i("irradianceMap", 5);

		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		Shader equiToCubemapShader(equiToCube_vert_src, equiToCube_frag_src, false);
		Shader skyboxShader(skybox_vert_shader, skybox_frag_shader, false);
		Shader convolveShader(hemisphere_convolver_vert_src, hemisphere_convolver_frag_src, false);

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
			glm::vec3(10,  10, 2.0f)
		};

		float lastLightRotationAngle = 0.0f;

		// ------------------- Rendering Equirectangular map to cubemap --------------------------------
		constexpr int numEnvMaps = 1;
		GLuint environmentMaps[numEnvMaps];

		glm::mat4 envProjectionMat = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		const GLuint FBOSize = 512;

		//see ShadowMapping_PointShadows_Basic.cpp for explanation for discussion about why these are created the way they are.
		std::vector<glm::mat4> viewMatrices;
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, -1, 0)));
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0, -1, 0)));
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 0, 1)));
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0, 0, -1)));
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0, -1, 0)));
		viewMatrices.emplace_back(glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0, -1, 0)));

		GLuint captureFBO, captureRBO; //RBO is for depth attachment -- may not be necessary
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, FBOSize, FBOSize);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
		auto genEnvironmentFloatCubeMapTexture = [](unsigned int width_lambda, unsigned int height_lambda) -> GLuint {
			GLuint environmentCubeMap;
			glGenTextures(1, &environmentCubeMap);
			glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubeMap);
			for (size_t face = 0; face < 6; ++face)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F, width_lambda, height_lambda, 0, GL_RGB, GL_FLOAT, nullptr);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			return environmentCubeMap;
		};

		//load environment map
		SimpleCubeMesh cubeMesh;
		glViewport(0, 0, FBOSize, FBOSize);
		equiToCubemapShader.use();
		equiToCubemapShader.setUniform1i("equirectangularHDRImage", 9);
		equiToCubemapShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(envProjectionMat));
		glActiveTexture(GL_TEXTURE9);
		for (size_t i = 0; i < sizeof(environmentMaps) / sizeof(GLuint); ++i)
		{
			glBindTexture(GL_TEXTURE_2D, hdr_newportTexture);
			environmentMaps[i] = genEnvironmentFloatCubeMapTexture(FBOSize, FBOSize);
			for (int face = 0; face < 6; ++face)
			{
				equiToCubemapShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(viewMatrices[face]));

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, environmentMaps[i], 0);
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cerr << "error on setup of environment HDR framebuffer" << std::endl; }

				glClearColor(0, 0, 0, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				cubeMesh.render();
			}
		}

		//--------------------------------- CONVOLVE ENVIRONMENT MAPS -------------------------------------------
		convolveShader.use();
		convolveShader.setUniform1i("environmentMap", 9);
		convolveShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(envProjectionMat));
		glActiveTexture(GL_TEXTURE9);
		GLuint irradianceMaps[numEnvMaps];
		constexpr unsigned int convolveSize = 32;

		glViewport(0, 0, convolveSize, convolveSize);
		for (size_t i = 0; i < sizeof(irradianceMaps) / sizeof(GLuint); ++i)
		{
			irradianceMaps[i] = genEnvironmentFloatCubeMapTexture(convolveSize, convolveSize);
			
			//bind after texture generation since it inherently requires binding to cubemap
			glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMaps[i]);
			std::cout << "convolving environment map:" << i << std::endl;
			for (int face = 0; face < 6; ++face)
			{
				convolveShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(viewMatrices[face]));
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, irradianceMaps[i], 0);
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cerr << "error on setup of convolve HDR framebuffer" << std::endl; }
				glClearColor(0, 0, 0, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				cubeMesh.render();
			}
		}

		std::cout << std::endl; 
		//restore usage of default framebuffer;
		glViewport(0, 0, width, height);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// ------------------- END FRAMEBUFFERS --------------------------------



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

		//textures bindings probably changed, fix up before rendering
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedoTexure);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, normalTexture);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, metalicTexture);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, roughnessTexture);

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

			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMaps[activeEnvironmentMap]);

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

			//render skybox
			glDepthFunc(GL_LEQUAL);
			skyboxShader.use();
			skyboxShader.setUniform1i("skybox", 6);
			skyboxShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			skyboxShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMaps[activeEnvironmentMap]);
			//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMaps[activeEnvironmentMap]);
			cubeMesh.render();
			glDepthFunc(GL_LESS);

			glfwSwapBuffers(window);
			glfwPollEvents();

		}
		//TODO: clean up gl resources (vaos, textures, etc)

		glfwTerminate();
	}
}

int main()
{
	true_main();
}