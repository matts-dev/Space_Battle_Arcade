
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include "Libraries/stb_image.h"
#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"
#include "ReferenceCode/OpenGL/InputTracker.h"
#include "ReferenceCode/OpenGL/Utilities/Lighting/DirectionalLight.h"
#include "ReferenceCode/OpenGL/Utilities/Lighting/ConeLight.h"
#include "ReferenceCode/OpenGL/Utilities/Lighting/PointLight.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "ReferenceCode/OpenGL/ImportingModels/Models/Model.h"
#include <glm/gtc/matrix_transform.hpp>
#include "ReferenceCode/OpenGL/Utilities/SphereMesh.h"
#include <complex>

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
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					//fragNormal = mat3(transpose(inverse(model))) * normal;
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

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
				
				out vec2 interpTexCoords;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					interpTexCoords = texCoords;
				}
			)";
	const char* lstage_LIGHTVOLUME_frag_shader_src = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D albedo_specs;

				uniform vec3 camPos;
				uniform float width_pixels = 0;
				uniform float height_pixels = 0;
				
				struct PointLight
				{
					vec3 position;
					vec3 ambientIntensity;	vec3 diffuseIntensity;	vec3 specularIntensity;
					float constant;			float linear;			float quadratic;
				};	

				uniform PointLight pointLights; 
				
				void main(){
					//FIGURE OUT SCREEN COORDINATES
					//gl_FragCoord is window space, eg a 800x600 would be range [0, 800] and [0, 600]; 
					//so to convert to UV range (ie [0, 1]), we divide teh frag_coord by the pixel values
					vec2 screenCoords = vec2(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels);


					//LOAD FROM GBUFFER (When rendering from sphere, we need to convert back to screen coords; alternatively this might could be done with perspective matrix but this way avoids matrix multiplication at expense of uniforms)
					vec3 fragPosition = texture(positions, screenCoords).rgb;
					vec3 fragNormal = normalize(texture(normals, screenCoords).rgb);
					vec3 color = texture(albedo_specs, screenCoords).rgb;
					float specularStrength = texture(albedo_specs, screenCoords).a;

					//LOAD FROM GBUFFER
					//vec3 fragPosition = texture(positions, interpTexCoords).rgb;
					//vec3 fragNormal = normalize(texture(normals, interpTexCoords).rgb);
					//vec3 color = texture(albedo_specs, interpTexCoords).rgb;
					//float specularStrength = texture(albedo_specs, interpTexCoords).a;

					//STANDARD LIGHT EQUATIONS
					PointLight light = pointLights; 

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 toReflection = reflect(-toLight, fragNormal);
					vec3 toView = normalize(camPos - fragPosition);

					vec3 ambientLight = light.ambientIntensity * color;

					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * color;

					float specularAmount = pow(max(dot(toView, toReflection), 0), 32); //shinnyness will need to be embeded in a gbuffer
					vec3 specularLight = light.specularIntensity* specularAmount * specularStrength;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);

					fragmentColor = vec4(lightContribution, 0.0f);
		
					//debug getting appropriate screen coords
					//fragmentColor = vec4(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels, 0.0, 1.0f);
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


	bool rotateLight = true;


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

	bool bDrawLightVolumes = false;
	void processInput(GLFWwindow* window)
	{
		static int initializeOneTimePrintMsg = []() -> int {
			std::cout << "Press R to toggle light rotation" << std::endl
				<< "Press L to draw the light volume bounding sphere (they're pretty huge)" << std::endl
				<< "" << std::endl
				<< std::endl;
			return 0;
		}();

		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_R))
		{
			rotateLight = !rotateLight;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_L))
		{
			bDrawLightVolumes = !bDrawLightVolumes;
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

		//float vertices[] = {
		//	// positions______    ___normals _______   texture coords
		//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		//	0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		//	0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		//	0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		//	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		//	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

		//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
		//	0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
		//	0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		//	0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		//	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
		//	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

		//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		//	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		//	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		//	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		//	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		//	0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		//	0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		//	0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		//	0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		//	0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		//	0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		//	0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		//	0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		//	0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		//	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		//	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

		//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		//	0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		//	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		//	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		//	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
		//	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
		//};
		float vertices[] = {
		    // Back face
		    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
		     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right         
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
		    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
		    // Front face
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
		     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
		     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
		    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
		    // Left face
		    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
		    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
		    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
		    // Right face
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
		     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right         
		     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
		     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left     
		    // Bottom face
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
		     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
		     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
		    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
		    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
		    // Top face
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
		     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right     
		     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
		    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
		    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // bottom-left        
		};

		Model meshModel("Models/nanosuit/nanosuit.obj");
		std::vector<ModelInstance> models;
		constexpr float positionOffset = 1.0f;
		constexpr int modelsPerRow = 10;
		constexpr int numModels = 50;
		for (int i = 0, row = 0, col = 0; i < numModels; ++i)
		{
			ModelInstance modelInstance;
			modelInstance.transform.position = glm::vec3(positionOffset * col, 0, -positionOffset * row);
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

		GLuint lampVAO;
		glGenVertexArrays(1, &lampVAO);
		glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);//use same vertex data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

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


		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);
		Shader postProcessShader(render_quad_shader_vert_src, render_quad_shader_frag_src, false);
		postProcessShader.use();
		postProcessShader.setUniform1i("renderTexture", 0);


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
				"pointLights", //true, 0
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(2.3f, 1.3f, -4.0f),
				"pointLights", //true, 1
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(4.0f, 2.0f, -6.0f),
				"pointLights",//true, 2
			},
			{
				pntAmbient,
				pntDiffuse,
				pntSpecular,
				plghtConstant,
				plghtLinear,
				plghtQuadratic,
				glm::vec3(0.f, 1.0f, -3.f),
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

		GLuint lAttachment_depthRBO;
		glGenRenderbuffers(1, &lAttachment_depthRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, lAttachment_depthRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, lAttachment_depthRBO);

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
			float maxLight = std::fmaxf(color.r, std::fmax(color.g, color.b));

			//float distance = (-Kl + std::sqrt(Kl*Kl - 4*Kq*(Kc-((256.0f/5.0f)* maxLight) ) ) / (2 * Kq)
			float A = Kq;
			float B = Kl;
			float C = Kc - ((256.0f / 5.0f) * maxLight);
			float distance = (-B + std::sqrt(B*B - 4 * A*C)) / (2*A);

			//for tutorial simplicity, just store this as a public variable
			light.lightRadius = distance;
		}


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

			// -------------------------- LIGHTING STAGE ---------------------------------------------------
			lightingStage_LVOLUME_Shader.use();
			lightingStage_LVOLUME_Shader.setUniform3f("camPos", camPos);
			//for (size_t i = 0; i < numPointLights; ++i)
			//{
			//	pointLights[i].applyUniforms(lightingStage_LVOLUME_Shader);
			//}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gAttachment_position);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gAttachment_normals);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);

			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDepthMask(GL_FALSE);

			//draw quad for lighting
			//glBindVertexArray(quadVAO);
			//glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad);

			//draw each sphere as to only apply lighting where the sphere is present
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			lightingStage_LVOLUME_Shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			lightingStage_LVOLUME_Shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			lightingStage_LVOLUME_Shader.setUniform1f("width_pixels", (float)width);
			lightingStage_LVOLUME_Shader.setUniform1f("height_pixels", (float)height);
			for (size_t i = 0; i < numPointLights; ++i)
			{
				ColoredPointLight& light = pointLights[i];

				//note: lights have been disabled as arrays for my light volumes; so they must be updated one at a time.
				light.applyUniforms(lightingStage_LVOLUME_Shader);

				glm::mat4 sphereModelMatrix;
				sphereModelMatrix = glm::translate(sphereModelMatrix, light.getPosition());
				sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(light.lightRadius));
				lightingStage_LVOLUME_Shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
				sphereMesh.render();
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			// --------------------- FORWARD SHADING COMPONENT -----------------------

			//copy over depth information so lights do not render on top of deferred shading objects
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer);

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

				//debuging -- visualize light volume
				if (bDrawLightVolumes)
				{
					glm::mat4 sphereModelMatrix;
					sphereModelMatrix = glm::translate(sphereModelMatrix, targetLight.getPosition());
					sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(targetLight.lightRadius));
					lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));

					//seems that lines will flicker on backface culling, but doesn't seem to have any effect on solid rendering geometry; so should be fine.
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
					sphereMesh.render();
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}
			}


			// --------------------------- POST PROCESSING --------------------------- 
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, gAttachment_normals);
			//glBindTexture(GL_TEXTURE_2D, gAttachment_position);
			//glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);
			glBindTexture(GL_TEXTURE_2D, lAttachment_lighting);

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


//int main()
//{
//	true_main();
//}