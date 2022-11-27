#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include<cmath>

#include "ReferenceCode/OpenGL/GettingStarted/Camera/CameraFPS.h"
#include "ReferenceCode/OpenGL/InputTracker.h"
#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"
#include <glm/gtx/quaternion.hpp>
#include <tuple>
#include <array>
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SHDebugUtils.h"
#include <functional>

namespace
{
	const char* litObjectShaderVertSrc = R"(
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
	const char* litObjectShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				uniform vec3 objectColor;
				uniform vec3 lightPosition;
				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform float ambientStrength = 0.1f; 
				uniform vec3 objColor = vec3(1,1,1);

				void main(){
					vec3 ambientLight = ambientStrength * lightColor;					

					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 normal = normalize(fragNormal);									//interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * lightColor;

					float specularStrength = 0.5f;
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 toReflection = reflect(-toView, normal);							//reflect expects vector from light position
					float specularAmount = pow(max(dot(toReflection, toLight), 0), 32);
					vec3 specularLight = specularStrength * lightColor * specularAmount;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * objectColor;
					
					fragmentColor = vec4(lightContribution, 1.0f);
				}
			)";
	const char* lightLocationShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* lightLocationShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";

	const char* Dim3DebugShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec4 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * position;
				}
			)";
	const char* Dim3DebugShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

	struct ColumnBasedTransform
	{
		glm::vec3 position = { 0, 0, 0 };
		glm::quat rotQuat; //identity quaternion; see conversion function if you're unfamiliar with quaternions
		glm::vec3 scale = { 1, 1, 1 };

		glm::mat4 getModelMatrix()
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = model * glm::toMat4(rotQuat);
			model = glm::scale(model, scale);
			return model;
		}
	};

	void printVec3(glm::vec3 v)
	{
		std::cout << "[" << v.x << ", " << v.y << ", " << v.z << "]";
	}

	void drawDebugLine(
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection
	)
	{
		/* This method is extremely slow and non-performant; use only for debugging purposes */
		static Shader shader(Dim3DebugShaderVertSrc, Dim3DebugShaderFragSrc, false);

		shader.use();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		shader.setUniform3f("color", color);

		//basically immediate mode, should be very bad performance
		GLuint tmpVAO, tmpVBO;
		glGenVertexArrays(1, &tmpVAO);
		glBindVertexArray(tmpVAO);
		float verts[] = {
			pntA.x,  pntA.y, pntA.z, 1.0f,
			pntB.x, pntB.y, pntB.z, 1.0f
		};
		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_LINES, 0, 2);

		glDeleteVertexArrays(1, &tmpVAO);
		glDeleteBuffers(1, &tmpVBO);
	}

	class IOpenGLDemo
	{
	public:
		IOpenGLDemo(int width, int height) {}
		virtual void tickGameLoop(GLFWwindow* window) = 0;
		virtual void handleModuleFocused(GLFWwindow* window) = 0;
	};

	/** Represents a top level object*/
	class GameEntity
	{

	};

	////////////////////////////////////////////////////////////////////////////////////////////
	// In this demo, the axis aligned bounding box (AABB) is the same as the shape itself.
	// So, just transforming the points the shape's transform will be appropriate to figure out
	// its oriented bounding box (OBB). In real applications with non-cube shapes, the OBB will
	// need to be pre-calculated at start up and then have transform applied to the OBB, 
	// rather than applying the transform to the AABB directly. 
	////////////////////////////////////////////////////////////////////////////////////////////
	struct CubeEntity_Ref : public GameEntity
	{
	public:
		CubeEntity_Ref(ColumnBasedTransform& inTransform)
			: transform(inTransform)
		{
		}

		/** see class note above about direct transforming of AABB */
		std::array<glm::vec4, 8> getOBB()
		{
			glm::mat4 xform = transform.getModelMatrix();
			std::array<glm::vec4, 8> OBB =
			{
				xform * AABB[0],
				xform * AABB[1],
				xform * AABB[2],
				xform * AABB[3],
				xform * AABB[4],
				xform * AABB[5],
				xform * AABB[6],
				xform * AABB[7]
			};
			return OBB;
		}

		/** axis aligned bounding box(AABB) will be oriented bounding box(OBB) when transformed */
		const std::array<glm::vec4, 8> AABB =
		{
			glm::vec4(-0.5f, -0.5f, -0.5f,	1.0f),
			glm::vec4(-0.5f, -0.5f, 0.5f,	1.0f),
			glm::vec4(-0.5f, 0.5f, -0.5f,	1.0f),
			glm::vec4(-0.5f, 0.5f, 0.5f,	1.0f),
			glm::vec4(0.5f, -0.5f, -0.5f,	1.0f),
			glm::vec4(0.5f, -0.5f, 0.5f,	1.0f),
			glm::vec4(0.5f, 0.5f, -0.5f,	1.0f),
			glm::vec4(0.5f, 0.5f, 0.5f,     1.0f),
		};

	public: 
		//these are public for having a of simple demo and logic being code below 
		//rather than having a proper class set up with manipulating member functions
		//in real applications, do properly encapsulate this stuff! I merely don't want the reader
		//to have to jump around the source file to see what is happening. 
		ColumnBasedTransform& transform;
		std::unique_ptr<SH::HashEntry<GameEntity>> spatialHashEntry;
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	// class used to hold state of demo
	////////////////////////////////////////////////////////////////////////////////////////////
	class VisualizeSpatialHash final : public IOpenGLDemo
	{
		//Cached Window Data
		int height;
		int width;
		float lastFrameTime = 0.f;
		float deltaTime = 0.0f;

		//OpenGL data
		GLuint cubeVAO, cubeVBO;

		//Shape Data
		ColumnBasedTransform redCubeTransform;
		ColumnBasedTransform blueCubeTransform;
		const ColumnBasedTransform defaultBlueCubeTransform = { { 0,0,0 }, {}, { 1,1,1 } };
		const ColumnBasedTransform defaultRedCubeTransform = { { 5,0,5 }, {}, { 1,1,1 } };
		const glm::vec3 blueCubeColor{ 0, 0, 1 };
		const glm::vec3 redCubeColor{ 1, 0, 0 };
		CubeEntity_Ref redEntity{redCubeTransform};
		CubeEntity_Ref blueEntity{blueCubeTransform};

		//State
		CameraFPS camera{ 45.0f, -90.f, 0.f };
		ColumnBasedTransform lightTransform;
		glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };
		Shader objShader{ litObjectShaderVertSrc , litObjectShaderFragSrc, false };
		Shader lampShader{ lightLocationShaderVertSrc, lightLocationShaderFragSrc, false };
		Shader debugGridShader{ SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false };

		//Modifiable State
		ColumnBasedTransform* transformTarget = &redCubeTransform;
		float moveSpeed = 3;
		float rotationSpeed = 45.0f;
		bool bEnableCollision = true;
		bool bBlockCollisionFailures = true;
		bool bTickUnitTests = false;
		bool bUseCameraAxesForObjectMovement = true;
		bool bEnableAxisOffset = false;
		bool bRenderHashGrid = true;
		bool bRenderOccupiedCells = true;
		glm::vec3 axisOffset{ 0, 0.005f, 0 };
		glm::vec3 cachedVelocity;

		SH::SpatialHashGrid<GameEntity> spatialHash{ glm::vec4{1,1,1,1} };


	public:
		VisualizeSpatialHash(int width, int height)
			: IOpenGLDemo(width, height)
		{
			using glm::vec2;
			using glm::vec3;
			using glm::mat4;

			camera.setPosition(0.0f, 0.0f, 3.0f);
			this->width = width;
			this->height = height;

			camera.setPosition(-7.29687f, 3.22111f, 5.0681f);
			camera.setYaw(-22.65f);
			camera.setPitch(-15.15f);

			/////////////////////////////////////////////////////////////////////
			// OpenGL/Collision Object Creation
			/////////////////////////////////////////////////////////////////////

			//unit create cube (that matches the size of the collision cube)
			float vertices[] = {
				//x     y       z      _____normal_xyz___
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
				0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
				0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
				-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
				-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

				0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
				0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
				0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
				0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
				0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
				0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
				0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
				0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
			};
			glGenVertexArrays(1, &cubeVAO);
			glBindVertexArray(cubeVAO);

			glGenBuffers(1, &cubeVBO);
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glBindVertexArray(0);

			redCubeTransform = defaultRedCubeTransform;
			blueCubeTransform = defaultBlueCubeTransform;

			lightTransform.position = glm::vec3(5, 3, 0);
			lightTransform.scale = glm::vec3(0.1f);

			////////////////////////////////////////////////////////////
			// initialization state
			////////////////////////////////////////////////////////////
			redEntity.spatialHashEntry = spatialHash.insert(redEntity, redEntity.getOBB());
			blueEntity.spatialHashEntry = spatialHash.insert(blueEntity, blueEntity.getOBB());

			{ //scoped unique_ptr
				//quick test that RAII is working
				//also testing that same-item hash removes don't clear other entries for same item
				std::unique_ptr<SH::HashEntry<GameEntity>> RAIITest = spatialHash.insert(redEntity, redEntity.getOBB());
			}
		}

		~VisualizeSpatialHash()
		{
			glDeleteVertexArrays(1, &cubeVAO);
			glDeleteBuffers(1, &cubeVBO);
		}

		virtual void handleModuleFocused(GLFWwindow* window)
		{
			camera.exclusiveGLFWCallbackRegister(window);
		}

		virtual void tickGameLoop(GLFWwindow* window)
		{
			using glm::vec2; using glm::vec3; using glm::mat4;

			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			//update spatial hashes since objects may have moved
			redEntity.spatialHashEntry = spatialHash.insert(redEntity, redEntity.getOBB());
			blueEntity.spatialHashEntry = spatialHash.insert(blueEntity, blueEntity.getOBB());

			mat4 view = camera.getView();
			mat4 projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(width) / height, 0.1f, 100.0f);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (bRenderHashGrid)
			{
				SH::drawAABBGrid<GameEntity>(spatialHash, glm::vec3(0.2f), debugGridShader, glm::mat4(1.0f), view, projection);

				glDepthFunc(GL_ALWAYS);
				////test drawing mechanics
				//std::vector<glm::ivec3> cells = {
				//	//{1, 1, 1},
				//	//{-1,-1, -1},
				//	{0,0,0}
				//};
				//SH::drawCells(cells, spatialHash.gridCellSize, glm::vec3(1, 0, 0), debugGridShader, glm::mat4(1.0f), view, projection);

				//draw result of hash lookup to test hashing behavior, rather than directly use cells on the HashEntry object
				std::vector<std::shared_ptr<SH::GridNode<GameEntity>>> itemsInCells;
				spatialHash.lookupNodesInCells(*redEntity.spatialHashEntry, itemsInCells);
				if (itemsInCells.size() > 1)
				{
					std::cerr << "detecting more than 1 entry in red cubes calls! there are only 2 objects in this demo" << std::endl;
				}

				//----------------------------------------------------------------------------------
				auto comparisonClosure = [](const glm::ivec3& a, const glm::ivec3& b) { 
					//set equal iff !(a < b) && !(b < a)
					
					//strict weak ordering for x
					if (a.x < b.x) return true;
					if (b.x < a.x) return false;

					//a.x == b.x
					if (a.y < b.y) return true;
					if (b.y < a.y) return false;

					//a.y == b.y
					if (a.z < b.z) return true;
					if (b.z < a.z) return false;
					
					//signals equivalence
					return false;
				};
				std::set<glm::vec3, std::function<bool (const glm::ivec3&, const glm::ivec3& ) >> redCellLocations(comparisonClosure);
				std::vector<std::shared_ptr<const SH::HashCell<GameEntity>>> redCells;
				spatialHash.lookupCellsForEntry(*redEntity.spatialHashEntry, redCells);
				for (std::shared_ptr<const SH::HashCell<GameEntity>>& cell : redCells)
				{
					redCellLocations.insert(cell->location);
				}

				std::set<glm::vec3, std::function<bool(const glm::ivec3&, const glm::ivec3&) >> blueCellLocations(comparisonClosure);
				std::vector<std::shared_ptr<const SH::HashCell<GameEntity>>> blueCells;
				spatialHash.lookupCellsForEntry(*blueEntity.spatialHashEntry, blueCells);
				for (std::shared_ptr<const SH::HashCell<GameEntity>>& cell : blueCells)
				{
					blueCellLocations.insert( cell->location);
				}

				std::vector<glm::ivec3> redCellLocationsVec;
				std::vector<glm::ivec3> blueCellLocationsVec;
				std::vector<glm::ivec3> overlapCellLocationsVec;
				for (const glm::vec3& redLoc : redCellLocations)
				{
					//cell overlap, mark it as a different color
					auto blueElementIter = blueCellLocations.find(redLoc);
					if (blueElementIter != blueCellLocations.end())
					{
						blueCellLocations.erase(blueElementIter);
						overlapCellLocationsVec.push_back(redLoc);
					}
					else
					{
						redCellLocationsVec.push_back(redLoc);
					}
				}
				for (const glm::vec3& blueLoc : blueCellLocations)
				{
					blueCellLocationsVec.push_back(blueLoc);
				}
				if(bRenderOccupiedCells)
				{
					SH::drawCells(redCellLocationsVec, spatialHash.gridCellSize, glm::vec3(1, 0, 0), debugGridShader, glm::mat4(1.0f), view, projection);
					SH::drawCells(blueCellLocationsVec, spatialHash.gridCellSize, glm::vec3(0, 0, 1), debugGridShader, glm::mat4(1.0f), view, projection);
					SH::drawCells(overlapCellLocationsVec, spatialHash.gridCellSize, glm::vec3(1, 0, 1), debugGridShader, glm::mat4(1.0f), view, projection);
				}

				glDepthFunc(GL_LESS);
				//------------------------------------------------------
			}

			glBindVertexArray(cubeVAO);
			{//render light
				mat4 model = lightTransform.getModelMatrix();
				lampShader.use();
				lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				lampShader.setUniform3f("lightColor", lightColor);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			objShader.use();
			objShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			objShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			objShader.setUniform3f("lightPosition", lightTransform.position);
			objShader.setUniform3f("lightColor", lightColor);
			objShader.setUniform3f("cameraPosition", camera.getPosition());

			{ //render red cube
				//resist temptation to update collision transform here, collision should be separated from rendering (for mtv corrections)
				mat4 model = redCubeTransform.getModelMatrix();
				objShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				objShader.setUniform3f("objectColor", redCubeColor);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			{// render blue cube
				//resist temptation to update collision transform here, collision should be separated from rendering (for mtv corrections)
				mat4 model = blueCubeTransform.getModelMatrix();
				objShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				objShader.setUniform3f("objectColor", blueCubeColor);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

	private:

		inline bool isZeroVector(const glm::vec3& testVec)
		{
			constexpr float EPSILON = 0.001f;
			return glm::length2(testVec) < EPSILON * EPSILON;
		}

		void processInput(GLFWwindow* window)
		{
			static int initializeOneTimePrintMsg = []() -> int {
				std::cout << "Hold ALT to move objects with WASD-EQ" << std::endl
					<< " Hold ALT + CONTROL to rotate objects with WASD-EQ" << std::endl
					<< " Press T to toggle which the object is moved " << std::endl
					<< " Press V to toggle locking translations(and rotations) to camera (easier movements) " << std::endl
					<< " Press P to print debug information" << std::endl
					<< " Press R to reset object positions to default." << std::endl
					<< " Press 9/0 to decrease/increase scale" << std::endl
					<< " " << std::endl
					<< std::endl;
				return 0;
			}();


			using glm::vec3; using glm::vec4;
			static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
			input.updateState(window);

			if (input.isKeyJustPressed(window, GLFW_KEY_ESCAPE))
			{
				glfwSetWindowShouldClose(window, true);
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_T))
			{
				transformTarget = transformTarget == &blueCubeTransform ? &redCubeTransform : &blueCubeTransform;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_V))
			{
				bUseCameraAxesForObjectMovement = !bUseCameraAxesForObjectMovement;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_M))
			{
				bEnableAxisOffset = !bEnableAxisOffset;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_H))
			{
				bRenderHashGrid = !bRenderHashGrid;
				bRenderOccupiedCells= !bRenderOccupiedCells;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_R))
			{
				redCubeTransform = defaultRedCubeTransform;
				blueCubeTransform = defaultBlueCubeTransform;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_C))
			{
				bEnableCollision = !bEnableCollision;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_C))
			{
				bEnableCollision = !bEnableCollision;
			}
			const glm::vec3 scaleSpeedPerSec(1, 1, 1);
			if (input.isKeyDown(window, GLFW_KEY_9))
			{
				vec3 delta = -scaleSpeedPerSec * deltaTime;
				transformTarget->scale += delta;
			}
			if (input.isKeyDown(window, GLFW_KEY_0))
			{
				transformTarget->scale += scaleSpeedPerSec * deltaTime;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_P))
			{
				using std::cout; using std::endl;
				cout << "camera position: ";  printVec3(camera.getPosition()); cout << " yaw:" << camera.getYaw() << " pitch: " << camera.getPitch() << endl;
				cout << "camera front:"; printVec3(camera.getFront()); cout << " right:"; printVec3(camera.getRight()); cout << " up:"; printVec3(camera.getUp()); cout << endl;
				auto printXform = [](ColumnBasedTransform& transform) {
					glm::vec3 rotQuat{ transform.rotQuat.x, transform.rotQuat.y, transform.rotQuat.z };
					cout << "pos:";  printVec3(transform.position); cout << " rotation:"; printVec3(rotQuat); cout << "w" << transform.rotQuat.w << " scale:"; printVec3(transform.scale); cout << endl;
				};
				cout << "red object =>"; printXform(redCubeTransform);
				cout << "blue object =>"; printXform(blueCubeTransform);

				spatialHash.logDebugInformation();
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_U))
			{
				bTickUnitTests = !bTickUnitTests;
				if (bTickUnitTests)
				{
				}
				else
				{
				}
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_L))
			{
				//profiler entry point
				int profiler_helper= 5;
				++profiler_helper;
			}
			// -------- MOVEMENT -----------------
			if (!(input.isKeyDown(window, GLFW_KEY_LEFT_ALT) || input.isKeyDown(window, GLFW_KEY_LEFT_SHIFT)))
			{
				camera.handleInput(window, deltaTime);
			}
			else
			{
				bool bUpdateMovement = false;
				vec3 movementInput = vec3(0.0f);
				vec3 cachedTargetPos = transformTarget->position;

				/////////////// W /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_W))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), camera.getRight()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), vec3(1, 0, 0)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += camera.getUp();
						}
						else
						{
							movementInput += vec3(0, 1, 0);
						}
					}
				}
				/////////////// S /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_S))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), camera.getRight()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), vec3(1, 0, 0)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += -camera.getUp();
						}
						else
						{
							movementInput += vec3(0, -1, 0);
						}
					}
				}
				/////////////// Q /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_Q))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), -camera.getFront()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), vec3(0, 0, 1)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += -camera.getFront();
						}
						else
						{
							movementInput += vec3(0, 0, 1);
						}
					}
				}
				/////////////// E /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_E))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), -camera.getFront()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), vec3(0, 0, 1)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += camera.getFront();
						}
						else
						{
							movementInput += vec3(0, 0, -1);
						}
					}
				}
				/////////////// A /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_A))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							vec3 toTarget = transformTarget->position - camera.getPosition();
							vec3 adjustedUp = glm::normalize(glm::cross(camera.getRight(), glm::normalize(toTarget)));
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), adjustedUp) * transformTarget->rotQuat;
							//glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), camera.getUp()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(-rotationSpeed * deltaTime), glm::vec3(0, 0, 1)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += -camera.getRight();
						}
						else
						{
							movementInput += vec3(-1, 0, 0);
						}
					}
				}
				/////////////// D /////////////// 
				if (input.isKeyDown(window, GLFW_KEY_D))
				{
					if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
					{
						if (bUseCameraAxesForObjectMovement)
						{
							vec3 toTarget = transformTarget->position - camera.getPosition();
							vec3 adjustedUp = glm::normalize(glm::cross(camera.getRight(), glm::normalize(toTarget)));
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), adjustedUp) * transformTarget->rotQuat;
							//glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), camera.getUp()) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
						else
						{
							glm::quat newRotation = glm::angleAxis(glm::radians(rotationSpeed * deltaTime), glm::vec3(0, 0, 1)) * transformTarget->rotQuat;
							transformTarget->rotQuat = newRotation;
						}
					}
					else
					{
						if (bUseCameraAxesForObjectMovement)
						{
							movementInput += camera.getRight();
						}
						else
						{
							movementInput += vec3(1, 0, 0);
						}
					}
				}
				//probably should use epsilon comparison for when opposite buttons held, but this should cover compares against initialization
				if (movementInput != vec3(0.0f))
				{
					movementInput = glm::normalize(movementInput);
					cachedVelocity = deltaTime * moveSpeed * movementInput;
					transformTarget->position += cachedVelocity;
				}
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_N))
			{
			}

			if (input.isKeyJustPressed(window, GLFW_KEY_UP))
			{
			}
			else if (input.isKeyJustPressed(window, GLFW_KEY_DOWN))
			{
			}
			else if (input.isKeyJustPressed(window, GLFW_KEY_LEFT))
			{
			}
			else if (input.isKeyJustPressed(window, GLFW_KEY_RIGHT))
			{
			}

		}


	};

	void true_main()
	{
		using glm::vec2;
		using glm::vec3;
		using glm::mat4;

		int width = 1200;
		int height = 800;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		VisualizeSpatialHash demo(width, height);
		demo.handleModuleFocused(window);

		/////////////////////////////////////////////////////////////////////
		// Game Loop
		/////////////////////////////////////////////////////////////////////
		while (!glfwWindowShouldClose(window))
		{
			demo.tickGameLoop(window);
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