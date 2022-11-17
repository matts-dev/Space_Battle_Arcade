
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
#include "../../Utilities/SphereMesh.h"
#include "../../Utilities/MeshInstance.h"
#include "../../Utilities/SimpleCubeMesh.h"

namespace
{
	CameraFPS camera(45.0f, -90.f, 0.f);

	float lastFrameTime = 0.f;
	float deltaTime = 0.0f;

	float pitch = 0.f;
	float yaw = -90.f;

	float FOV = 45.0f;

	/** A simple material struct for the material values provides; shininess is a non-standard approach I believe.*/
	struct SimpleMaterial
	{
		glm::vec3 ambientColor;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;

		SimpleMaterial() {};

		SimpleMaterial(glm::vec3 ambientColor, glm::vec3 diffuseColor, glm::vec3 specularColor, float shininess)
		{
			this->ambientColor = ambientColor;
			this->diffuseColor = diffuseColor;
			this->specularColor = specularColor;
			this->shininess = shininess;
		}

		void apply(Shader& shader)
		{
			shader.setUniform3f("material.ambientColor", ambientColor.x, ambientColor.y, ambientColor.z);
			shader.setUniform3f("material.diffuseColor", diffuseColor.x, diffuseColor.y, diffuseColor.z);
			shader.setUniform3f("material.specularColor", specularColor.x, specularColor.y, specularColor.z);
			shader.setUniform1i("material.shininess", static_cast<int>(128 * shininess)); /* these material values expect shininess to be multipled by 128 */
		}
	};

	struct ShapeSwaper
	{
		MeshInstance* current;
		MeshInstance sphereModel;
		MeshInstance cubeModel;

		glm::vec3 pos{ 0,0,0 };
		glm::vec3 rot{ 0,0,0 };
		glm::vec3 scale{ 1,1,1 };
		SimpleMaterial material;

		ShapeSwaper(Mesh*sphere, Mesh*cube) : sphereModel(*sphere), cubeModel(*cube)
		{
			current = &sphereModel;
			refresh();
		}
		ShapeSwaper(const ShapeSwaper& toCopy) : sphereModel(toCopy.sphereModel), cubeModel(toCopy.cubeModel)
		{
			current = &sphereModel;
			this->pos = toCopy.pos;
			this->rot = toCopy.rot;
			this->scale = toCopy.scale;
			this->material = toCopy.material;
			refresh();
		}
		ShapeSwaper operator=(const ShapeSwaper& toCopy) = delete;

		void swap()
		{
			if (current == &sphereModel)
				current = &cubeModel;
			else
				current = &sphereModel;
			refresh();
		}

		void refresh()
		{
			current->setPosition(pos);
			current->setRotation(rot);
			current->setScale(scale);
		}
	};
	std::vector<ShapeSwaper> materialDemos;

	const char* vertex_shader_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);
				}
			)";
	const char* frag_shader_src = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					vec3 ambientColor;    //object's reflected color under ambient
					vec3 diffuseColor;    //object's reflected color under diffuse
					vec3 specularColor;   //light's color impact on object
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
				};	
				uniform Light light;

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;
				

				void main(){
					vec3 ambientLight = light.ambientIntensity * material.ambientColor;					

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 normal = normalize(fragNormal);														//interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * material.diffuseColor;

					vec3 toReflection = reflect(-toLight, normal);												//reflect expects vector from light position (tutorial didn't normalize this vector)
					vec3 toView = normalize(cameraPosition - fragPosition);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * material.specularColor;
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight);
					
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

	void swapAllShapes()
	{
		for (ShapeSwaper& material : materialDemos)
		{
			material.swap();
		}
	}

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

		if (input.isKeyJustPressed(window, GLFW_KEY_H))
		{
			swapAllShapes();
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
		int width = 1200;
		int height = 800;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);
		camera.setPosition(0, 0, 10);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		//GENERATE LAMP
		SimpleCubeMesh cubeMesh;
		MeshInstance lamp(cubeMesh);
		Transformable lampParent; //responsible for giving spinning behavior
		lampParent.addChild(&lamp);


		Shader shader(vertex_shader_src, frag_shader_src, false);
		Shader lampShader(lamp_vertex_shader_src, lamp_frag_shader_src, false);

		glEnable(GL_DEPTH_TEST);

		glm::vec3 lightcolor(1.0f, 1.0f, 1.0f);
		//glm::vec3 objectcolor(1.0f, 0.5f, 0.31f);
		glm::vec3 objectcolor(1.0f, 1.f, 1.f);

		glm::vec3 objectPos;
		glm::vec3 lightStart(1.2f, 1.0f, -10.0f);
		glm::vec3 lightPos(lightStart);

		SphereMesh sphereSource;

		MatrixStack mstack;

		//test transform hiearchy
		MeshInstance sphereParent(sphereSource);
		MeshInstance sphereA(sphereSource);
		MeshInstance sphereB(sphereSource);
		MeshInstance sphereC(sphereSource);
		sphereA.setPosition({ 2, 0, 0 });
		sphereB.setPosition({ 0, 2, 0 });
		sphereC.setPosition({ 0, 0, 2 });
		sphereParent.addChild(&sphereA);
		sphereParent.addChild(&sphereB);
		sphereParent.addChild(&sphereC);


		//create materials
		std::vector<SimpleMaterial> materials = {
			/* Table from http://devernay.free.fr/cours/opengl/materials.html */
			/*												------------ambient-----------		    -----------diffuse------------------------          ------------------specular-----------    ---shininess coeffient of 128---																												*/
			/*emerald		*/		SimpleMaterial(glm::vec3(0.0215f	,0.1745f	,0.0215f	),	glm::vec3(0.07568f	,0.61424f	,0.07568f	),		glm::vec3(0.633f		,0.727811f	,0.633f		),	0.6f		) ,
			/*jade			*/		SimpleMaterial(glm::vec3(0.135f		,0.2225f	,0.1575f	),	glm::vec3(0.54f		,0.89f		,0.63f		),		glm::vec3(0.316228f		,0.316228f	,0.316228f	),	0.1f		) ,
			/*obsidian		*/		SimpleMaterial(glm::vec3(0.05375f	,0.05f		,0.06625f	),	glm::vec3(0.18275f	,0.17f		,0.22525f	),		glm::vec3(0.332741f		,0.328634f	,0.346435f	),	0.3f		) ,
			/*pearl			*/		SimpleMaterial(glm::vec3(0.25f		,0.20725f	,0.20725f	),	glm::vec3(1.0f		,0.829f		,0.829f		),		glm::vec3(0.296648f		,0.296648f	,0.296648f	),	0.088f		) ,
			/*ruby			*/		SimpleMaterial(glm::vec3(0.1745f	,0.01175f	,0.01175f	),	glm::vec3(0.61424f	,0.04136f	,0.04136f	),		glm::vec3(0.727811f		,0.626959f	,0.626959f	),	0.6f		) ,
			/*turquoise		*/		SimpleMaterial(glm::vec3(0.1f		,0.18725f	,0.1745f	),	glm::vec3(0.396f	,0.74151f	,0.69102f	),		glm::vec3(0.297254f		,0.30829f	,0.306678f	),	0.1f		) ,
			/*brass			*/		SimpleMaterial(glm::vec3(0.329412f	,0.223529f	,0.027451f	),	glm::vec3(0.780392f	,0.568627f	,0.113725f	),		glm::vec3(0.992157f		,0.941176f	,0.807843f	),	0.21794872f	),
			/*bronze		*/		SimpleMaterial(glm::vec3(0.2125f	,0.1275f	,0.054f		),	glm::vec3(0.714f	,0.4284f	,0.18144f	),		glm::vec3(0.393548f		,0.271906f	,0.166721f	),	0.2f		) ,
			/*chrome		*/		SimpleMaterial(glm::vec3(0.25f		,0.25f		,0.25f		),	glm::vec3(0.4f		,0.4f		,0.4f		),		glm::vec3(0.774597f		,0.774597f	,0.774597f	),	0.6f		) ,
			/*copper		*/		SimpleMaterial(glm::vec3(0.19125f	,0.0735f	,0.0225f	),	glm::vec3(0.7038f	,0.27048f	,0.0828f	),		glm::vec3(0.256777f		,0.137622f	,0.086014f	),	0.1f		) ,
			/*gold			*/		SimpleMaterial(glm::vec3(0.24725f	,0.1995f	,0.0745f	),	glm::vec3(0.75164f	,0.60648f	,0.22648f	),		glm::vec3(0.628281f		,0.555802f	,0.366065f	),	0.4f		) ,
			/*silver		*/		SimpleMaterial(glm::vec3(0.19225f	,0.19225f	,0.19225f	),	glm::vec3(0.50754f	,0.50754f	,0.50754f	),		glm::vec3(0.508273f		,0.508273f	,0.508273f	),	0.4f		) ,
			/*black plastic	*/		SimpleMaterial(glm::vec3(0.0f		,0.0f		,0.0f		),	glm::vec3(0.01f		,0.01f		,0.01f		),		glm::vec3(0.50f			,0.50f		,0.50f		),	0.25f		) ,
			/*cyan plastic	*/		SimpleMaterial(glm::vec3(0.0f		,0.1f		,0.06f		),	glm::vec3(0.0f		,0.50980392f,0.50980392f),		glm::vec3(0.50196078f	,0.50196078f,0.50196078f),	0.25f		) ,
			/*green plastic	*/		SimpleMaterial(glm::vec3(0.0f		,0.0f		,0.0f		),	glm::vec3(0.1f		,0.35f		,0.1f		),		glm::vec3(0.45f			,0.55f		,0.45f		),	0.25f		) ,
			/*red plastic	*/		SimpleMaterial(glm::vec3(0.0f		,0.0f		,0.0f		),	glm::vec3(0.5f		,0.0f		,0.0f		),		glm::vec3(0.7f			,0.6f		,0.6f		),	0.25f		) ,
			/*white plastic	*/		SimpleMaterial(glm::vec3(0.0f		,0.0f		,0.0f		),	glm::vec3(0.55f		,0.55f		,0.55f		),		glm::vec3(0.70f			,0.70f		,0.70f		),	0.25f		) ,
			/*yellow plastic*/		SimpleMaterial(glm::vec3(0.0f		,0.0f		,0.0f		),	glm::vec3(0.5f		,0.5f		,0.0f		),		glm::vec3(0.60f			,0.60f		,0.50f		),	0.25f		) ,
			/*black rubber	*/		SimpleMaterial(glm::vec3(0.02f		,0.02f		,0.02f		),	glm::vec3(0.01f		,0.01f		,0.01f		),		glm::vec3(0.4f			,0.4f		,0.4f		),	0.078125f	) ,
			/*cyan rubber	*/		SimpleMaterial(glm::vec3(0.0f		,0.05f		,0.05f		),	glm::vec3(0.4f		,0.5f		,0.5f		),		glm::vec3(0.04f			,0.7f		,0.7f		),	0.078125f	) ,
			/*green rubber	*/		SimpleMaterial(glm::vec3(0.0f		,0.05f		,0.0f		),	glm::vec3(0.4f		,0.5f		,0.4f		),		glm::vec3(0.04f			,0.7f		,0.04f		),	0.078125f	) ,
			/*red rubber	*/		SimpleMaterial(glm::vec3(0.05f		,0.0f		,0.0f		),	glm::vec3(0.5f		,0.4f		,0.4f		),		glm::vec3(0.7f			,0.04f		,0.04f		),	0.078125f	) ,
			/*white rubber	*/		SimpleMaterial(glm::vec3(0.05f		,0.05f		,0.05f		),	glm::vec3(0.5f		,0.5f		,0.5f		),		glm::vec3(0.7f			,0.7f		,0.7f		),	0.078125f	) ,
			/*yellow rubber	*/		SimpleMaterial(glm::vec3(0.05f		,0.05f		,0.0f		),	glm::vec3(0.5f		,0.5f		,0.4f		),		glm::vec3(0.7f			,0.7f		,0.04f		),	0.078125f	)
			
		};

		//create demo objects for materials
		float spacing = 2.5f;
		float yStart = 5.f;
		float xStart = -6 * spacing / 2;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 6; ++j)
			{
				materialDemos.emplace_back( &sphereSource, &cubeMesh);
				int index = i * 6 + j;
				auto& inserted = materialDemos[index];
				inserted.pos = glm::vec3(j*spacing + xStart, -i * spacing + yStart, -5);
				inserted.refresh();
				inserted.material = materials[index];
			}
		}

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
			
			//these materials were designed with white light with ambient and diffuse intensities at 1.0
			//lightcolor.x = static_cast<float>(sin(glfwGetTime() * 2.0f) / 2 + 0.5f);
			//lightcolor.y = static_cast<float>(sin(glfwGetTime() * 0.7f) / 2 + 0.5f);
			//lightcolor.z = static_cast<float>(sin(glfwGetTime() * 1.3f) / 2 + 0.5f);
			glm::vec3 diffuseColor = lightcolor;// *diffuseStrength;
			glm::vec3 ambientColor = diffuseColor;// *ambientStrength; //this is the tutorial did, seems like we should use lightcolor instead of diffuseColor.

			glm::mat4 model;
			float lightXRotation = rotateLight ? 100 * currentTime : 0;
			model = glm::rotate(model, glm::radians(rotateLight ? 100 * currentTime : 0), glm::vec3(0, 1.f, 0)); //apply rotation leftmost (after translation) to give it an orbit
			model = glm::translate(model, lightStart);
			model = glm::scale(model, glm::vec3(0.2f));
			lightPos = glm::vec3(model * glm::vec4(0, 0, 0, 1));

			lamp.setPosition(lightStart);
			lampParent.setRotation({ 0, lightXRotation, 0 });
			lamp.setScale(glm::vec3(0.2f));

			lampShader.use();
			lampShader.setUniform3f("lightColor", lightcolor.x, lightcolor.y, lightcolor.z);
			lampParent.render(projection, view, mstack, lampShader);

			//draw object
			model = glm::mat4(1.f); //set model to identity matrix
			model = glm::translate(model, objectPos);
			model = glm::rotate(model, yRotation, glm::vec3(0.f, 1.f, 0.f));
			shader.use();
			shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
			shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			shader.setUniform3f("light.position", lightPos.x, lightPos.y, lightPos.z);

			//tweak parameters
			shader.setUniform3f("material.ambientColor", objectcolor.x, objectcolor.y, objectcolor.z);
			shader.setUniform3f("material.diffuseColor", objectcolor.x, objectcolor.y, objectcolor.z);
			shader.setUniform3f("material.specularColor", objectcolor.x * 0.5f, objectcolor.y * 0.5f, objectcolor.z * 0.5f);
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

			materials[0].apply(shader);
			sphereParent.setPosition({ -30, 0, -20 });
			sphereParent.setRotation({ glfwGetTime() * 55, glfwGetTime() * 20, glfwGetTime() * 40 });
			sphereParent.render(projection, view, mstack, shader);

			for (ShapeSwaper& materialDemo : materialDemos)
			{
				materialDemo.material.apply(shader);
				materialDemo.current->render(projection, view, mstack, shader);
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

/* Table from http://devernay.free.fr/cours/opengl/materials.html
emerald			0.0215		0.1745		0.0215		0.07568		0.61424		0.07568		0.633		0.727811	0.633		0.6
jade			0.135		0.2225		0.1575		0.54		0.89		0.63		0.316228	0.316228	0.316228	0.1
obsidian		0.05375		0.05		0.06625		0.18275		0.17		0.22525		0.332741	0.328634	0.346435	0.3
pearl			0.25		0.20725		0.20725		1			0.829		0.829		0.296648	0.296648	0.296648	0.088
ruby			0.1745		0.01175		0.01175		0.61424		0.04136		0.04136		0.727811	0.626959	0.626959	0.6
turquoise		0.1			0.18725		0.1745		0.396		0.74151		0.69102		0.297254	0.30829		0.306678	0.1
brass			0.329412	0.223529	0.027451	0.780392	0.568627	0.113725	0.992157	0.941176	0.807843	0.21794872
bronze			0.2125		0.1275		0.054		0.714		0.4284		0.18144		0.393548	0.271906	0.166721	0.2
chrome			0.25		0.25		0.25		0.4			0.4			0.4			0.774597	0.774597	0.774597	0.6
copper			0.19125		0.0735		0.0225		0.7038		0.27048		0.0828		0.256777	0.137622	0.086014	0.1
gold			0.24725		0.1995		0.0745		0.75164		0.60648		0.22648		0.628281	0.555802	0.366065	0.4
silver			0.19225		0.19225		0.19225		0.50754		0.50754		0.50754		0.508273	0.508273	0.508273	0.4
black plastic	0.0			0.0			0.0			0.01		0.01		0.01		0.50		0.50		0.50		.25
cyan plastic	0.0			0.1			0.06		0.0			0.50980392	0.50980392	0.50196078	0.50196078	0.50196078	.25
green plastic	0.0			0.0			0.0			0.1			0.35		0.1			0.45		0.55		0.45		.25
red plastic		0.0			0.0			0.0			0.5			0.0			0.0			0.7			0.6			0.6			.25
white plastic	0.0			0.0			0.0			0.55		0.55		0.55		0.70		0.70		0.70		.25
yellow plastic	0.0			0.0			0.0			0.5			0.5			0.0			0.60		0.60		0.50		.25
black rubber	0.02		0.02		0.02		0.01		0.01		0.01		0.4			0.4			0.4			.078125
cyan rubber		0.0			0.05		0.05		0.4			0.5			0.5			0.04		0.7			0.7			.078125
green rubber	0.0			0.05		0.0			0.4			0.5			0.4			0.04		0.7			0.04		.078125
red rubber		0.05		0.0			0.0			0.5			0.4			0.4			0.7			0.04		0.04		.078125
white rubber	0.05		0.05		0.05		0.5			0.5			0.5			0.7			0.7			0.7			.078125
yellow rubber	0.05		0.05		0.0			0.5			0.5			0.4			0.7			0.7			0.04		.078125
*/