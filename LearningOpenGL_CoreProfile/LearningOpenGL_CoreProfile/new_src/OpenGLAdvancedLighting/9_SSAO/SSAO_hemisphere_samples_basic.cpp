//There's a lot going on in this file since it is ported from the deferred render
//instead of new secions     being   delinated by ---------
// new sections relevant to ssao are delinated by ///////////

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
#include "../../Utilities/SphereMesh.h"
#include <complex>
#include <random>
#include <limits>

namespace
{
	CameraFPS camera(45.0f, -90.f, 0.f);

	float lastFrameTime = 0.f;
	float deltaTime = 0.0f;

	float pitch = 0.f;
	float yaw = -90.f;

	float FOV = 45.0f;

	const char* gstage_vertex_shader_src = R"(
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
					//in real world applications, reduce number of matrix multiplies (or even just do them on CPU) 
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(view * model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(view * model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

					interpTextCoords = textureCoordinates;
				}
			)";
	const char* gstage_frag_shader_src = R"(
			#version 330 core

				layout (location = 0) out vec3 position;
				layout (location = 1) out vec3 normal;
				layout (location = 2) out vec4 albedo_spec;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					int shininess;
				};
				uniform Material material;			

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				void main(){
					position.rgb = fragPosition;
					normal.rgb = fragNormal;
					albedo_spec.rgb = texture(material.texture_diffuse0, interpTextCoords).rgb;
					albedo_spec.a = texture(material.texture_specular0, interpTextCoords).r;
				}
			)";

	
	const char* lstage_LIGHTVOLUME_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 texCoords;				

				uniform mat4 projection;
				uniform mat4 view;
				uniform mat4 model;

				struct PointLight //change warning: this is also defined in the fragment shader
				{
					vec3 position;
					vec3 ambientIntensity;	vec3 diffuseIntensity;	vec3 specularIntensity;
					float constant;			float linear;			float quadratic;
				};	
				uniform PointLight pointLights; 

				out vec2 interpTexCoords;
				out vec4 pointLightPos_VS;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					interpTexCoords = texCoords;

					pointLightPos_VS = view * vec4(pointLights.position, 1.0f);
				}
			)";
	const char* lstage_LIGHTVOLUME_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;
				in vec4 pointLightPos_VS;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D albedo_specs;
				uniform sampler2D ambient_occlusion;

				uniform float width_pixels = 0;
				uniform float height_pixels = 0;

				uniform bool bEnableSSAO = true;
				
				struct PointLight //change warning: this is also defined in the vertex shader to get position into view space
				{
					vec3 position;
					vec3 ambientIntensity;	vec3 diffuseIntensity;	vec3 specularIntensity;
					float constant;			float linear;			float quadratic;
				};	
				uniform PointLight pointLights; 
				
				void main(){
					//FIGURE OUT SCREEN COORDINATES
					//gl_FragCoord is window space, eg a 800x600 would be range [0, 800] and [0, 600]; 
					//so to convert to UV range (ie [0, 1]), we divide the frag_coord by the pixel values
					vec2 screenCoords = vec2(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels);

					//LOAD FROM GBUFFER (When rendering from sphere, we need to convert back to screen coords; alternatively this might could be done with perspective matrix but this way avoids matrix multiplication at expense of uniforms)
					vec3 fragPosition = texture(positions, screenCoords).rgb;
					vec3 fragNormal = normalize(texture(normals, screenCoords).rgb);
					vec3 color = texture(albedo_specs, screenCoords).rgb;
					float specularStrength = texture(albedo_specs, screenCoords).a;
					float ambientOcclusionAllowance = bEnableSSAO ? texture(ambient_occlusion, screenCoords).r : 1.0f;

					//STANDARD LIGHT EQUATIONS
					PointLight light = pointLights; 

					vec3 toLight = normalize(pointLightPos_VS.xyz - fragPosition);
					vec3 toReflection = reflect(-toLight, fragNormal);
					vec3 toView = normalize(-fragPosition); //in view space, camPos = 0,0,0; so camPos - fragPos is just -fragPos; so we get this one for free :) 

					vec3 ambientLight = light.ambientIntensity * color * ambientOcclusionAllowance;

					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * color;

					float specularAmount = pow(max(dot(toView, toReflection), 0), 32); //shinnyness will need to be embeded in a gbuffer
					vec3 specularLight = light.specularIntensity* specularAmount * specularStrength;

					float distance = length(pointLightPos_VS.xyz - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);

					fragmentColor = vec4(lightContribution, 0.0f);

					//debug visualizations
					//fragmentColor = vec4(toLight, 1.0f);
					//fragmentColor = vec4(toReflection, 1.0f);
					//fragmentColor = vec4(fragNormal, 1.0f);
					//fragmentColor = vec4(fragPosition, 1.0f);
					//fragmentColor = vec4(diffuseLight, 1.0f);
					//fragmentColor = vec4(vec3(attenuation), 1.0f);
					//fragmentColor = vec4(vec3(distance/1000.0f), 1.0f);
					//fragmentColor = vec4(pointLightPos_VS.xyz, 1.0f);

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

	const char* stencilwriter_vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					//render to screen space so stencil buffer is filled in 
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* stencilwriter_frag_shader_src = R"(
				#version 330 core
				void main(){
					//do nothing, let stencil buffer be updated through glStencilFunc/glStencilOp
				}
			)";
	const char* render_quad_shader_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 textureCoords;				

				out vec2 interpTextureCoords;				

				void main(){
					gl_Position = vec4(position, 1);
					interpTextureCoords = textureCoords;
				}
			)";
	const char* render_quad_shader_frag_src = R"(
				#version 330 core

				out vec4 fragmentColor;

				in vec2 interpTextureCoords;

				uniform sampler2D renderTexture;
				
				void main(){
					fragmentColor = texture(renderTexture, interpTextureCoords);
				}
			)";
	const char* SSAO_quad_shader_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 textureCoords;				
				out vec2 interpTextureCoords;				
				void main(){
					gl_Position = vec4(position, 1);
					interpTextureCoords = textureCoords;
				}
			)";
	const char* SSAO_quad_shader_frag_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 interpTextureCoords;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D randomNoiseVectors;

				uniform float radius = 0.5f; //artistic knob to influence ssao; via increasing/decreasing the sample positions radius in hemisphere
				uniform float rangeCheckStrength = 1.0f; //multiplier that higher numbers decrease the range of ssao samples; thinking of this a clip value -- where increasing value has more clipping on range
				uniform mat4 projection;
				
				const int NUM_SSAO_KERNEL_SAMPLES = 64;
				uniform vec3 samples[NUM_SSAO_KERNEL_SAMPLES];

				void main(){
					
					//rendering whole quad as screen, so we can use interpolated texture coordinates to access gbuffer (as opposed to method when rendering spheres)
					vec3 position = texture(positions, interpTextureCoords).rgb;
					vec3 normal = texture(normals, interpTextureCoords).rgb;

					//------GET RANDOM VEC---------
					//noise pattern is only a 4x4 texture, we need this to tile
					//tutorial hard-codes resolution into shader, but I'd rather be a little more dynamic.
					vec2 screenSizePixels = textureSize(positions, 0);
					vec2 noiseTexSizeInPixels = textureSize(randomNoiseVectors, 0);
					vec2 conversion = screenSizePixels/noiseTexSizeInPixels;
					vec2 tiledTexCoords = conversion * interpTextureCoords;
					//vec2 tiledTexCoords = ((vec2(1200.0f,800.0f)) / 4.0f )* interpTextureCoords; //tutorial's method is hardcoded to screen size
					vec3 randomXYVec = normalize(texture(randomNoiseVectors, tiledTexCoords).xyz);

					//------CALC TBN---------
					//this is the method presented in the tutorial series, but I believe it has some issues
					//this method assumes that the two vectors won't be orthogonal (in which case there isn't a non-zero projection)
					//it also assumes that the normal and the random vector are no co-linear (which will not be great for crossproduct, will return 0)
					vec3 tangent = normalize(normal - dot(normal, randomXYVec) * randomXYVec); //my way
					//vec3 tangent = normalize(randomXYVec - dot(normal, randomXYVec) * normal); //tutorial way
					vec3 bitangent = cross(normal, tangent);

					mat3 TBN = mat3(tangent, bitangent, normal);
					
					//------ OCCLUSION ------------
					float occlusion = 0.0f;
					for(int i = 0; i < NUM_SSAO_KERNEL_SAMPLES; ++i){
						//use the sample to find depth and determine if occlusion should be incremented
						vec3 sampleLoc_VS = (TBN * samples[i]) * radius  + position;
						
						//convert to screen/clip space and sample depth
						vec4 sampleLoc_CS = projection * vec4(sampleLoc_VS, 1.0f);
						sampleLoc_CS.xyz /= sampleLoc_CS.w;
						sampleLoc_CS.xyz = (sampleLoc_CS.xyz * 0.5f) + 0.5f; //convert to UV coordinates [-1, 1] => [0, 1]

						float sampleDepth = texture(positions, sampleLoc_CS.xy).z; //rename to lookupDepth?
						
						//locations are in view space, 0,0,0 is camera;
						//camera looks in -z direction; that means these z values have negative values (since they're relative to camera)
						//so... keep in mind that:
						//more negative means farther away.  Which may explain why the '>' seems backwards.
						float bias = 0.0f;

						//A: I feel like this method makes more intuitive sense, it checks if the actual fragment's position closer than something sampled from around it; but it is not what the tutorial checks
						//float occludeFactor = sampleDepth > position.z + bias ? 1.0f : 0.0f; //ie if the sampleDepth is less negative that the fragment's position //causes weird artifact, might be interesting to explore

						//B: tutorial's method compares if the generated sample's z value is within geometry
						float occludeFactor = sampleDepth >= (sampleLoc_VS.z + bias) ? 1.0f : 0.0f; //ie if the sampleDepth is less negative that the fragment's position

						//prevent geometry that is very far away from contributing to ambient occlusion 
						float value_smallWhenFar_LargeWhenNear =  radius / (rangeCheckStrength * abs(position.z - sampleDepth));    //eg radius = 5,   5/abs(4-5) = 5           5/abs(5-100) = 0.052
						float rangeCheck = smoothstep(0.0f, 1.0f, value_smallWhenFar_LargeWhenNear);	//this is like a LERP, but has a slight curve to it; think of it as LERP

						//range check prevents *abrupt* disable of AmbientOcclusion for distance geometry
						occlusion += occludeFactor * rangeCheck;
					}
					
					//1. it makes sense that occlusion would be on the range [0, 1.0f] since we're going to be multiplying it against a color
					//    We can seeing what fraction of samples were occluded (occlusion/num_samples)
					//2.transform occlusion into something we can multiply the ambient color by
					//    low occlusion should have a value of 1 to show the full ambient color
					occlusion = 1.0f - (occlusion / NUM_SSAO_KERNEL_SAMPLES);

					fragmentColor = vec4(vec3(occlusion), 1.0f);

					//debug visualizations
					//fragmentColor = vec4(position, 1.0f);		//X
					//fragmentColor = vec4(normal, 1.0f);		//X
					//fragmentColor = vec4(samples[0], 1.0f);   //X color is consistent
					//fragmentColor = vec4(samples[1], 1.0f);   //X color is consistent
					//fragmentColor = vec4(randomXYVec, 1.0f);  //x
					//fragmentColor = vec4(tangent, 1.0f);    
					//fragmentColor = vec4(bitangent, 1.0f);    
					//fragmentColor = vec4(db_sampleLoc_VS, 1.0f);	
					//fragmentColor = vec4(vec3(db_sampleDepth), 1.0f);
 					

				}
			)";

	const char* SSAO_SIMPLEBLUR_quad_shader_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 textureCoords;				

				out vec2 interpTextureCoords;				

				void main(){
					gl_Position = vec4(position, 1);
					interpTextureCoords = textureCoords;
				}
			)";
	const char* SSAO_SIMPLEBLUR_quad_shader_frag_src = R"(
				#version 330 core

				out float fragmentRed; 

				in vec2 interpTextureCoords;
				uniform sampler2D renderTexture;

				void main(){
					vec2 texelSize = 1.0f / textureSize(renderTexture, 0);
					float fragmentResult = 0.0f;

					for(int x = -2; x < 2; ++x){
						for(int y = -2; y < 2; ++y){
							vec2 offset = vec2(float(x), float(y)) * texelSize;
						
							fragmentResult += texture(renderTexture, offset + interpTextureCoords).r;
						}
					}

					fragmentRed = fragmentResult / (4.0 * 4.0);
				}
			)";

	const char* infinite_depth_filler_shader_vert_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 textureCoords;				

				out vec2 interpTextureCoords;				

				void main(){
					//force depth to be 1 after perspective divide (z/w is 1/1 in this case)
					//this will require changing depth function to LEQUAL instead of default LESS because 1 is clipped
					gl_Position = vec4(position.xy, 1, 1);

					interpTextureCoords = textureCoords;
				}
			)";
	const char* infinite_depth_filler_shader_frag_src = R"(
				#version 330 core
				layout (location = 0) out vec3 gPosition;

				in vec2 interpTextureCoords;

				uniform float infinity;

				void main(){
					//SSAO actually uses position depths within the position buffer, so we cannot just simply set the depth
					gPosition = vec3(0, 0, -infinity);

					gl_FragDepth = 1.0f;
				}
			)";

	const int NUM_SSAO_SAMPLES = 64;

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
		const float getAttenuationConstant() { return attenuationConstant; }
		const float getAttenuationLinear() { return attenuationLinear; }
		const float getAttenuationQuadratic() { return attenuationQuadratic; }

		//for tutorial simplicity, just store this as a public variable
		float lightRadius = 1.0;
	};

	bool rotateLight = true;
	bool bDrawLightVolumes = false;
	bool bDebugLightVolumes = false;
	int displayBuffer = 4;
	float ssaoRadiusTweak = 0.5f;
	const float radiusIncrementAmount = 0.5f;
	float rangeCheckStrength = 8.0f;
	const float rangeCheckIncrementAmount = 4.0f;
	bool bEnableSSAO = true;
	bool bUseInfiniteDepthCorrection = true;


	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press R to toggle light rotation" << std::endl
				<< "Press L to draw the light volume bounding sphere (they're pretty huge)" << std::endl
				<< "Press B to decrease light volume size for debugging" << std::endl
				<< "Press UP/DOWN to increase/decrease SSAO sample radius" << std::endl
				<< "Press F to toggle background depth fill to maximum value (solves infinity background depth of 0 issue for ssao)" << std::endl
				<< "GBuffer/Render display: 1=normals, 2=positions, 3=diffuse-spec, 4=SSAO preblur, 5=SSAO blur, 6=Final Lighting Pass" << std::endl
				<< std::endl;
			return 0;
		}();

		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			ssaoRadiusTweak += deltaTime * radiusIncrementAmount;
			std::cout << "SSAO sample radius: " << ssaoRadiusTweak << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			ssaoRadiusTweak -= deltaTime * radiusIncrementAmount;
			std::cout << "SSAO sample radius: " << ssaoRadiusTweak << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			rangeCheckStrength -= rangeCheckIncrementAmount * deltaTime;
			std::cout << "Range check strength: " << rangeCheckStrength << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			rangeCheckStrength += rangeCheckIncrementAmount * deltaTime;
			std::cout << "Range check strength: " << rangeCheckStrength << std::endl;
		}

		if (input.isKeyJustPressed(window, GLFW_KEY_O))
		{
			bEnableSSAO = !bEnableSSAO;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_F))
		{
			bUseInfiniteDepthCorrection = !bUseInfiniteDepthCorrection;
		}

		for (int key = GLFW_KEY_1; key < GLFW_KEY_7; ++key)
		{
			if (input.isKeyJustPressed(window, key))
			{
				displayBuffer = key - GLFW_KEY_0;
			}
		}

		if (input.isKeyJustPressed(window, GLFW_KEY_R))
		{
			rotateLight = !rotateLight;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_L))
		{
			bDrawLightVolumes = !bDrawLightVolumes;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_B))
		{
			bDebugLightVolumes = !bDebugLightVolumes;
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
			// x    y      z       s      t   nX     nY     nZ
		    // Back face 
		    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f, 0.0f, -1.0f,// Bottom-left
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f,// top-right
		     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 0.0f, -1.0f,// bottom-right         
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f,// top-right
		    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f, 0.0f, -1.0f,// bottom-left
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 0.0f, -1.0f,// top-left
		    // Front face
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f,// bottom-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f,// bottom-right
		     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-right
		     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-right
		    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f,// bottom-left
		    // Left face
		    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f,// top-right
		    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,// top-left
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f, 0.0f, 0.0f,// bottom-left
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f, 0.0f, 0.0f,// bottom-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,// bottom-right
		    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f,// top-right
		    // Right face
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,// top-left
		     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f,// bottom-right
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f,// top-right         
		     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f,// bottom-right
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,// top-left
		     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,// bottom-left     
		    // Bottom face
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, -1.0f, 0.0f,// top-right
		     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f, -1.0f, 0.0f,// top-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, -1.0f, 0.0f,// bottom-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, -1.0f, 0.0f,// bottom-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f, -1.0f, 0.0f,// bottom-right
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, -1.0f, 0.0f,// top-right
		    // Top face
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,// top-left
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,// bottom-right
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f, 0.0f,// top-right     
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,// bottom-right
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,// top-left
		    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f// bottom-left        
		};


		//textures
		GLuint cubeDiffuseMap = textureLoader("Textures/container2.png", GL_TEXTURE0);
		GLuint cubeSpecularMap = textureLoader("Textures/container2_specular.png", GL_TEXTURE1);
		std::vector<Transform> cubeTransforms;
		cubeTransforms.push_back(Transform{ {0,-1,0},{0,0,0},{30,1,30} });
		cubeTransforms.push_back(Transform{ { -5,0,0 },{ 0,0,0 },{ 1,1,1 } }); //lone box
		cubeTransforms.push_back(Transform{ { -3,0,0.5f },{ 0,45,0 },{ 1,1,1 } }); //bottom stack
		cubeTransforms.push_back(Transform{ { -3,1,0.5f },{ 0,0,0 },{ 1,1,1 } });  //top stack
		cubeTransforms.push_back(Transform{ { -6.5f,0,0.5f },{ 0,0,0 },{ 1,5,10 } });  //wall-leftside
		cubeTransforms.push_back(Transform{ { -2,0,-3.0f },{ 0,0,0 },{ 10,5,1 } });  //wall-back

		//Model meshModel("Models/nanosuit/nanosuit.obj");
		Model meshModel("Models/tie_fighter/tie_1blend.obj");

		std::vector<ModelInstance> models;
		constexpr float positionOffset = 1.0f;
		constexpr int modelsPerRow = 10;
		constexpr int numModels = 50;
		for (int i = 0, row = 0, col = 0; i < numModels; ++i)
		{
			ModelInstance modelInstance;
			modelInstance.transform.position = glm::vec3(positionOffset * col, 0, -positionOffset * row);
			modelInstance.transform.position -= glm::vec3(1, 0, -1); //displace slightly
			//modelInstance.transform.scale = glm::vec3(0.1f); //for nanosuit
			modelInstance.transform.scale = glm::vec3(0.05f); //for tiefighter
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

		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(2);

		//shaders
		Shader geometricStageShader(gstage_vertex_shader_src, gstage_frag_shader_src, false);
		geometricStageShader.use();
		geometricStageShader.setUniform1i("material.texture_diffuse0", 0);
		geometricStageShader.setUniform1i("material.texture_specular0", 1);

		//more efficient with complex scenes
		Shader lightingStage_LVOLUME_Shader(lstage_LIGHTVOLUME_vertex_shader_src, lstage_LIGHTVOLUME_frag_shader_src, false);
		lightingStage_LVOLUME_Shader.use();
		lightingStage_LVOLUME_Shader.setUniform1i("positions", 0);
		lightingStage_LVOLUME_Shader.setUniform1i("normals", 1);
		lightingStage_LVOLUME_Shader.setUniform1i("albedo_specs", 2);
		lightingStage_LVOLUME_Shader.setUniform1i("ambient_occlusion", 3);


		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);
		lampShader.use();

		//just for writing stencil buffer data
		Shader stencilWriterShader(stencilwriter_vertex_shader_src, stencilwriter_frag_shader_src, false);
		stencilWriterShader.use();

		Shader postProcessShader(render_quad_shader_vert_src, render_quad_shader_frag_src, false);
		postProcessShader.use();
		postProcessShader.setUniform1i("renderTexture", 0);

		Shader ssaoShader(SSAO_quad_shader_vert_src, SSAO_quad_shader_frag_src, false);
		ssaoShader.use();
		ssaoShader.setUniform1i("positions", 0);
		ssaoShader.setUniform1i("normals", 1);
		ssaoShader.setUniform1i("randomNoiseVectors", 2);

		Shader ssaoBlur(SSAO_SIMPLEBLUR_quad_shader_vert_src, SSAO_SIMPLEBLUR_quad_shader_frag_src, false);
		ssaoBlur.use();
		ssaoBlur.setUniform1i("renderTexture", 0);

		Shader infiniteDepthCorrection(infinite_depth_filler_shader_vert_src, infinite_depth_filler_shader_frag_src, false);
		float infinity = std::numeric_limits<float>::infinity();
		infiniteDepthCorrection.setUniform1f("infinity", infinity); //only later versions of GLSL will give infinity when dividing by zero

		//for completeness, make sure each shader has a identity model matrix
		const glm::mat4 identityMatrix(1.0f);
		ssaoShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(identityMatrix));
		lightingStage_LVOLUME_Shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(identityMatrix));
		lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(identityMatrix));
		stencilWriterShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(identityMatrix));
		postProcessShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(identityMatrix));


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
		/*
		point light constants chart from Ogre3D engine
		***LIGHT VOLUME NOTICE*** this values seem to produce very large light volumes because of having such a small quadratic term

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
		float plghtConstant = 1.f;
		float plghtLinear = 0.7f;
		float plghtQuadratic = 1.8f;

		/*struct PointLight
		{
		glm::vec3 position;
		glm::vec3 ambientIntensity;v
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
				"pointLights", //true, 0
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(-5.3f, 2.3f, -2.0f),
				"pointLights", //true, 1
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				0.22f,
				0.20f,
				glm::vec3(-4.0f, 2.0f, 3.0f), 
				"pointLights",//true, 2
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(-1.f, 1.0f, 1.f),
				"pointLights",//true, 3
			}
		};
		const unsigned int numPointLights = sizeof(pointLights) / sizeof(ColoredPointLight);


		//--------------------------------------------------------------------------------------------
		GLuint gbuffer;
		glGenFramebuffers(1, &gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

		//positions buffer
		GLuint gAttachment_position;
		glGenTextures(1, &gAttachment_position);
		glBindTexture(GL_TEXTURE_2D, gAttachment_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, /**/GL_COLOR_ATTACHMENT0/**/, GL_TEXTURE_2D, gAttachment_position, 0);

		//normals buffer
		GLuint gAttachment_normals;
		glGenTextures(1, &gAttachment_normals);
		glBindTexture(GL_TEXTURE_2D, gAttachment_normals);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, /**/GL_COLOR_ATTACHMENT1/**/, GL_TEXTURE_2D, gAttachment_normals, 0);

		// ALBEDO (diffuse) + specular buffer
		GLuint gAttachment_albedospec;
		glGenTextures(1, &gAttachment_albedospec);
		glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, /**/ GL_COLOR_ATTACHMENT2 /**/, GL_TEXTURE_2D, gAttachment_albedospec, 0);

		GLuint depthAttachmentRBO;
		glGenRenderbuffers(1, &depthAttachmentRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, depthAttachmentRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthAttachmentRBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cout << "failure creating framebuffer" << std::endl;
		}

		GLuint gbufferRenderTargets[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, gbufferRenderTargets);

		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		//create a render quad
		float quadVertices[] = {
			//x,y,z         s,t
			-1, -1, 0,      0, 0,
			1, -1, 0,       1, 0,
			1,  1, 0,       1, 1,

			-1, -1, 0,      0, 0,
			1,  1, 0,       1, 1,
			-1,  1, 0,      0, 1
		};
		const size_t numVertsInQuad = (sizeof(quadVertices) / sizeof(float)) / 5;

		GLuint quadVAO;
		glGenVertexArrays(1, &quadVAO);
		glBindVertexArray(quadVAO);

		GLuint quad_VBO;
		glGenBuffers(1, &quad_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		//--------------------------------------------------------------------------------------------

		//-------------------------------- LBUFFER ---------------------------------------------------
		GLuint lbuffer;
		glGenFramebuffers(1, &lbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

		//positions buffer
		GLuint lAttachment_lighting;
		glGenTextures(1, &lAttachment_lighting);
		glBindTexture(GL_TEXTURE_2D, lAttachment_lighting);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lAttachment_lighting, 0);

		GLuint lAttachment_depthStencilRBO;
		glGenRenderbuffers(1, &lAttachment_depthStencilRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, lAttachment_depthStencilRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, lAttachment_depthStencilRBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cout << "failure creating lighting buffer" << std::endl;
		}

		//--------------------------------------------------------------------------------------------
		float lastLightRotationAngle = 0.0f;


		// ------------------------------------ LIGHT VOLUME PREP ------------------------------------ 
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		
		SphereMesh sphereMesh(0.095f);

		for (size_t i = 0; i < numPointLights; ++i)
		{
			ColoredPointLight& light = pointLights[i];

			//The light volume is a sphere that's radius is considered the maximum possible distance that the light has influence (ie lights objects up)
			//We can figure out this radius by solving for when the light's influence would be 0 due to being too far away (at least sort of).
			//since the attenuation function we used (light / (Constant + LinearConstant * d + QuadraticConstant * d^2) only approaches 0, we don't solve directly for 0;
			//instead we pick some very dark value and solve for that instead.
			//The tutorial recommends 5/256 as a illumination amount because it is near dark

			// Let's say that A = Constant + LinearConstant * d + QuadraticConstant * d^2; and I = light
			//		also that Kl = LienarConstant, Kq = QuadraticConstant, and Kc = Constant
			// thus, as our equation to solve we have we have we have:		(5/256) = light	/ A
			//		light is a fixed amount, but our distances in attenuation are not; let's solve for the distance
			//
			//	(5/256) = I / A
			//	A * (5/256) = I
			//	A * 5 = 256 * I
			//	A = (256/5) * I
			//	Kc + Kl*d + Kq*d^2 = (256/5) * I
			// Kc + Kl*d + Kq*d^2 - ((256/5) * I) = 0
			//
			// We can use the quadratic formula to solve for this
			// note that all the constants should be positive values
			// note also that we want a positive distance for the radius,
			// and because the first term of the quadratic equation is -B; we only want to add the sqrt(-B^2 - 4AC) (because sqrt will be positive and we don't want (negative - positive) because distance would then be negative
			//
			// Kc + Kl*d + Kq*d^2 - ((256/5) * I) = 0
			// A = Kq
			// B = Kl
			// C = Kc - (256/5)*I
			//
			// d = (-B +- sqrt(B^2 - 4AC))   /     (2A)
			//
			// d = (  -B  + sqrt( B^2  - 4*A *        C      ))   /     (2A)
			// d = ((-Kl) + sqrt(Kl*Kl - 4*Kq*(Kc - (256/5)*I))   /      2 * Kq
			glm::vec3 color = light.getDiffuseColor();
			float Kc = light.getAttenuationConstant();
			float Kl = light.getAttenuationLinear();
			float Kq = light.getAttenuationQuadratic();
			
			//take the strongest component as it will have the furthest attenuation distance
			//float maxLight = std::fmaxf(color.r, std::fmax(color.g, color.b)); //seems like suming up intensities gives better result when using many lights
			float maxLight = color.r + color.g + color.b;

			//float distance = (-Kl + std::sqrt(Kl*Kl - 4*Kq*(Kc-((256.0f/5.0f)* maxLight) ) ) / (2 * Kq)
			float A = Kq;
			float B = Kl;
			float C = Kc - ((256.0f / 5.0f) * maxLight);
			float distance = (-B + std::sqrt(B*B - 4 * A*C)) / (2*A);

			//for tutorial simplicity, just store this as a public variable
			light.lightRadius = distance;
			//light.lightRadius = 1; //DEBUG THAT ONLY FRAGMENTS CONTAINED IN VOLUME RENDERED; turn really low and make sure lighting looks clipped 
		}

		/////////////////////////////////////////////////////////////////////////////////////////////
		//CREATE SAMPLE LOCATIONS IN HEMISPHERE
		std::uniform_real_distribution<float> randomFloatRange_0_to_1(0.0f, 1.0f);
		std::default_random_engine generator;

		//predefine sample points for ssao depth checking; this is called a kernal
		//we basically pick random points within the hemisphere that will be used for the SSAO algorithm
		std::vector<glm::vec3> ssaoKernel;
		for (int i = 0; i < NUM_SSAO_SAMPLES; ++i)
		{
			//we want x and y to be in the range [-1,1];
			//we want z to be on the range [0, 1];
			//think of a circle, we're choosing a random xy somewhere in the 360 degrees.
			//as for z, we want it at some height within half of the sphere 
			float x = randomFloatRange_0_to_1(generator) * 2.0f - 1.0f; //[0, 1] ->  [0, 2] -> [-1, 1]
			float y = randomFloatRange_0_to_1(generator) * 2.0f - 1.0f;
			float z = randomFloatRange_0_to_1(generator);

			glm::vec3 samplePos(x, y, z);
			samplePos = glm::normalize(samplePos);

			//give the sample a random scale; think of this as pushing/pulling postion to/from the center of the hemisphere
			samplePos *= randomFloatRange_0_to_1(generator);

			auto lerp = [](float a, float b, float perc) {return a + perc * (b - a); }; //to intuitively understand this way of lerping, think of the floats as vectors (and think about what + and - do to vectors geometrically)

			//it would be nice if samples were more dense around the origin
			//we can make that happen using a curve; the goal is to have more samples at a lower value
			// 1- - - - - - - - - -
			// |                -*-
			// |              _-  -
			// |       ___--**    -
			// | ---***      ^    -
			// -------------------1. . . . . . . 2     
			//      curve: x^2
			//
			// the above curve has more density below 0.5 than above 0.5 over the entire range of 1;
			// the ^ marks the spot where it crosses 0.5
			// using such a curve, we can bias samples to be closer to the origin

			float samplePosInTotal = (float)i / NUM_SSAO_SAMPLES; //range [0, 1]; this will be our x in the x^2

			//bias the sample
			float biasToCenterWithScale = samplePosInTotal * samplePosInTotal;

			//we don't want to scale by 0, so set a limit to downscaling by 0.1f
			biasToCenterWithScale = lerp(0.1f, 1.0f, biasToCenterWithScale);
			samplePos *= biasToCenterWithScale;

			ssaoKernel.push_back(samplePos);
		}

		//send samples to the shader for storage
		ssaoShader.use();
		for (unsigned int i = 0; i < ssaoKernel.size(); ++i)
		{
			const glm::vec3& sample = ssaoKernel[i];

			std::string uniformName = std::string("samples[") + std::to_string(i) + std::string("]");
			ssaoShader.setUniform3f(uniformName.c_str(), sample);
		}


		//CREATE RANDOM ROTATIONS OF SAMPLE HEMISPHERES 
		//we can keep the sample numbers low if we randomly rotate the kernel
		//this will help prevent "banding" that occurs with low sample numbers
		//in the end, we will blur together result so using a randomness isn't too bad 

		std::vector<glm::vec3> noiseRotations;
		for (int i = 0; i < 16; ++i)
		{
			//the normal will be pointing in the +z direction in tangent space
			//ultimately, we will be creating a TBN matrix to transform the samples from tangent space to viewspace (the space we're doing lighting calculations in this time)
			//we can simulate rotating the samples by rotating the tangent space's x axis (ie the tangent axis)
			//the below code picks a random vector that will serve as this tangent; ultimately we will use the cross product to generate an orthonormal basis 
			//this isnt the exact vector we will use to create a tangent to the normal, but it will be used via the gram-schmidt process to construct a tangent
			glm::vec3 randomXYPlaneVector(
				randomFloatRange_0_to_1(generator) * 2.0f -1.0f, //transform [0, 1] to [-1, 1]
				randomFloatRange_0_to_1(generator) * 2.0f - 1.0f, //transform [0, 1] to [-1, 1]
				0
			);
			noiseRotations.push_back(randomXYPlaneVector);
		}

		//create a texture to hole the rotation noise vectors
		GLuint noiseRotationTexture;
		glGenTextures(1, &noiseRotationTexture);
		glBindTexture(GL_TEXTURE_2D, noiseRotationTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &noiseRotations[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


		////CREATE A FBO TO HOLD SSAO OCCLUSION FACTOR  ////
		GLuint ssaoFBO;
		glGenFramebuffers(1, &ssaoFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

		GLuint ssao_attachment;
		glGenTextures(1, &ssao_attachment);
		glBindTexture(GL_TEXTURE_2D, ssao_attachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr); //tutorial has pixel format as RGB, not red; perhaps mistake?
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, nullptr); //use extra color channels for debugging
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_attachment, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cout << "failure in SSAO frame buffer" << std::endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// SSAO BLUR BUFFER
		GLuint ssaoBlurFBO;
		glGenFramebuffers(1, &ssaoBlurFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

		GLuint ssaoBlur_colorAttachment;
		glGenTextures(1, &ssaoBlur_colorAttachment);
		glBindTexture(GL_TEXTURE_2D, ssaoBlur_colorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlur_colorAttachment, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cout << "failure in SSAO BLUR frame buffer" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		

		/////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////

		// --------------------------------------------------------------------------------------------
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);


			//prepare rotating light
			glm::mat4 model;
			lastLightRotationAngle = rotateLight ? 100 * currentTime : lastLightRotationAngle;
			model = glm::rotate(model, glm::radians(lastLightRotationAngle), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightStart);
			model = glm::scale(model, glm::vec3(0.2f));
			pointLights[0].setPosition(glm::vec3(model * glm::vec4(0, 0, 0, 1)));


			glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 view = camera.getView();
			glm::mat4 projection = glm::perspective(glm::radians(FOV), static_cast<float>(width) / height, 0.1f, 100.0f);

			// -------------------------- GEOMETRY STAGE ---------------------------------------------------
			geometricStageShader.use();
			const glm::vec3& camPos = camera.getPosition();
			geometricStageShader.setUniform3f("cameraPosition", camPos.x, camPos.y, camPos.z);
			geometricStageShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			geometricStageShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

			for (size_t i = 0; i < models.size(); ++i)
			{
				Transform& transform = models[i].transform;

				glm::mat4 model = glm::mat4(1.f); //set model to identity matrix
				model = glm::translate(model, transform.position);
				model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
				model = glm::scale(model, transform.scale);
				geometricStageShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));

				models[i].modelSource->draw(geometricStageShader);
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, cubeDiffuseMap);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, cubeSpecularMap);

			//draw cubes (share same VAO as lamp, don't mistake this for drawning lamp boxes)
			glBindVertexArray(lampVAO);
			for (size_t i = 0; i < cubeTransforms.size(); ++i)
			{
				Transform& transform = cubeTransforms[i];
				glm::mat4 model = glm::mat4(1.f); //set model to identity matrix
				model = glm::translate(model, transform.position);
				model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
				model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
				model = glm::scale(model, transform.scale);
				geometricStageShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}


			///////////////////////////////////////////////////
			//SSAO needs a backdrop for depth calculations to work properly
			//     if nothing is drawn, the depth is 0; causeing incorrect SSAO on geometry very close to camera (depth of 0 will be within the range check)
			//     using cube maps and setting depth to the maximum value will solve this issue for SSAO; but we can also solve it by manually setting the depth
			if (bUseInfiniteDepthCorrection)
			{
				glDepthFunc(GL_LEQUAL);
				infiniteDepthCorrection.use();
				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad);
				glDepthFunc(GL_LESS);
			}

			///////////////////////////////////////////////

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//--------------------------SSAO STAGE----------------------------------------------------------
			
			///// RENDER OCCLUSION 
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gAttachment_position);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gAttachment_normals);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseRotationTexture);

			ssaoShader.use();
			ssaoShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			ssaoShader.setUniform1f("radius", ssaoRadiusTweak);
			ssaoShader.setUniform1f("rangeCheckStrength", rangeCheckStrength);

			//draw the quad so SSAO is applied to entire viewing screen
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad);


			//// BLUR OCCLUSION
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			ssaoBlur.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ssao_attachment);

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad);

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			// -------------------------- LIGHTING STAGE ---------------------------------------------------

			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gAttachment_position);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gAttachment_normals);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, ssaoBlur_colorAttachment);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//---- COPY DEPTH ---- copy over depth information so lights do not render on top of deferred shading objects
			//this is needed for forward shading component and for **stencil testing light volumes**
			//		FYI tutorial warns about this type of copying might not be portable when copying to/from default framebuffer (ie 0) because the depth buffer formats must match
			//		(this si fine because we're copying between two FBs that we know match in terms of depth buffer configuration)
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);
			//----------------

			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_STENCIL_TEST);
			
			lightingStage_LVOLUME_Shader.use();
			//lightingStage_LVOLUME_Shader.setUniform3f("camPos", camPos); //now rendering in view space
			lightingStage_LVOLUME_Shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			lightingStage_LVOLUME_Shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lightingStage_LVOLUME_Shader.setUniform1f("width_pixels", (float)width);
			lightingStage_LVOLUME_Shader.setUniform1f("height_pixels", (float)height);
			lightingStage_LVOLUME_Shader.setUniform1i("bEnableSSAO", bEnableSSAO);
			

			stencilWriterShader.use();
			//stencilWriterShader.setUniform3f("camPos", camPos);
			stencilWriterShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			stencilWriterShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

			for (size_t i = 0; i < numPointLights; ++i)
			{
				ColoredPointLight& light = pointLights[i];
				float lightRadius = bDebugLightVolumes ? 1 : light.lightRadius;

				//note: lights have been disabled as arrays for my light volumes; so they must be updated one at a time.
				glm::mat4 sphereModelMatrix;
				sphereModelMatrix = glm::translate(sphereModelMatrix, light.getPosition());
				sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(lightRadius));
				stencilWriterShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));

				//------STENCIL PASS---------
				glDisable(GL_CULL_FACE);
				glClearStencil(0); //sets value for clearing stencil, for debugging you can change this value 
				glStencilMask(0xFF);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilFunc(GL_ALWAYS, 0, 0xFF); //always write stencil results

				//  light volume sphere                                   X
				//      _____			        _____	                 _____			            _____
				//    +  +1  +			      +  +1   +	               +   +0  +	              +   +1  +
				//  +          +		    +          +             +          +		        +     X    +
				// |            |		   |     X      |           |            |		       |            |
				//  +          +		    +          +             +          +		    --- +     vp   +  -----
				//    +  -1   +			      +       +	               +   +0  +		 	|     +      + not    |
				//      -----			        -----	                 -----			    |behind ----- rendered|
				//        X                                                                 |_____________________|
				//      vp                       vp                       vp
				//
				// X is object; vp is viewer's position. When stencil is 0, no lighting is applied
				//
				// stencil = 0;                stencil = +1              stencil = 0               stencil = +1
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP); //note this is "DEPTH FAILS", less intuitive than depth pass but works better
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP); //note: stencil does not havez negative numbers according to spec

				stencilWriterShader.use();
				sphereMesh.render(); 

				//------LIGHTING PASS--------
				glDisable(GL_DEPTH_TEST); //don't allow depth to stop rendering a sphere
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT); //use back of sphere for lighting so that if camera is inside sphere lighting is still rendered
				lightingStage_LVOLUME_Shader.use();
				lightingStage_LVOLUME_Shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
				light.applyUniforms(lightingStage_LVOLUME_Shader);
				glStencilMask(0); //disable writing
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				glStencilFunc(GL_NOTEQUAL, 0, 0xFF); //only write if passed behind front_face (-1) and infront of  back_face(+1); (-1 + 1 = 0);
				sphereMesh.render();
				glEnable(GL_DEPTH_TEST); //reenable
			}
			glDisable(GL_STENCIL_TEST);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glCullFace(GL_BACK);
			// --------------------- FORWARD SHADING COMPONENT -----------------------

			//draw light
			lampShader.use();
			lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			for (size_t i = 0; i < sizeof(pointLights) / sizeof(ColoredPointLight); ++i)
			{
				ColoredPointLight& targetLight = pointLights[i];
				glm::vec3 diffuseColor = targetLight.getDiffuseColor();
				glm::vec3 ambientColor = diffuseColor;

				glm::mat4 model;
				model = glm::translate(model, pointLights[i].getPosition());
				model = glm::scale(model, glm::vec3(0.2f));

				glm::vec3 lightColor = pointLights[i].getDiffuseColor();

				lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
				glBindVertexArray(lampVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);

				//debugging -- visualize light volume
				if (bDrawLightVolumes)
				{
					glm::mat4 sphereModelMatrix;
					float lightRadius = bDebugLightVolumes ? 1 : targetLight.lightRadius;
					sphereModelMatrix = glm::translate(sphereModelMatrix, targetLight.getPosition());
					sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(lightRadius));
					lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));

					//seems that lines will flicker on backface culling, but doesn't seem to have any effect on solid rendering geometry; so should be fine.
					glDisable(GL_CULL_FACE);
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
					sphereMesh.render();
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					glEnable(GL_CULL_FACE);
				}
			}


			// --------------------------- POST PROCESSING --------------------------- 
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			
			switch (displayBuffer)
			{
				case 1:
					glBindTexture(GL_TEXTURE_2D, gAttachment_normals);
					break;
				case 2:
					glBindTexture(GL_TEXTURE_2D, gAttachment_position);
					break;
				case 3:
					glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);
					break;
				case 4:
					glBindTexture(GL_TEXTURE_2D, ssao_attachment);
					break;
				case 5:
					glBindTexture(GL_TEXTURE_2D, ssaoBlur_colorAttachment);
					break;
				case 6:
					glBindTexture(GL_TEXTURE_2D, lAttachment_lighting);
					break;
			}

			postProcessShader.use();
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad);

			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glfwTerminate();

		//TODO clean up resources
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &lampVAO);
	}
}

/*
point light constants chart from Ogre3D engine
***LIGHT VOLUME NOTICE*** this values seem to produce very large light volumes because of having such a small quadratic term

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


int main()
{
	true_main();
}