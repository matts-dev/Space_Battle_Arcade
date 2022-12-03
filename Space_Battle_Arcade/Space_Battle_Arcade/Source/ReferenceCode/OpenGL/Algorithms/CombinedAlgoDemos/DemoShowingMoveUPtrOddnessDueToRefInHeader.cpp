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
#include <cstdint>
#include <random>

#include "ReferenceCode/OpenGL/Utilities/FrameRateDisplay.h"
#include "ReferenceCode/OpenGL/Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include <memory>

namespace A419
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

	/** Represents a top level object; simple struct for demo purposes*/
	struct CollisionEntity
	{
	public:
		explicit CollisionEntity(std::unique_ptr< SAT::Shape>&& managedShape, const ColumnBasedTransform& inTransform)
			: myShape(std::move(managedShape)), transform(inTransform)
		{
			if (!myShape)
			{
				std::cerr << "FAILURE: Game Entity created without a shape; aborting" << std::endl;
				exit(-1);
			}
		}

		//shape on this level prevents dynamic casts
		std::unique_ptr< SAT::Shape> myShape;
		ColumnBasedTransform transform;
		glm::vec3 velocity;
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	// In this demo, the axis aligned bounding box (AABB) is the same as the shape itself.
	// So, just transforming the points the shape's transform will be appropriate to figure out
	// its oriented bounding box (OBB). In real applications with non-cube shapes, the OBB will
	// need to be pre-calculated at start up and then have transform applied to the OBB, 
	// rather than applying the transform to the AABB directly. 
	////////////////////////////////////////////////////////////////////////////////////////////
	struct D_CollisionCubeEntity : public CollisionEntity
	{
	public:
		D_CollisionCubeEntity(const ColumnBasedTransform& inTransform)
			: CollisionEntity(std::make_unique<SAT::CubeShape>(), inTransform)
		{
			myShape->updateTransform(transform.getModelMatrix());
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
		glm::vec3 color;
		glm::vec3 gravityPnt;
		std::unique_ptr<SH::HashEntry<CollisionEntity>> spatialHashEntry;
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	// class used to hold state of demo
	////////////////////////////////////////////////////////////////////////////////////////////
	class UniquePtrOddnessDemo final : public IOpenGLDemo
	{
		//Cached Window Data
		int height;
		int width;
		float lastFrameTime = 0.f;
		float deltaTime = 0.0f;

		//OpenGL data
		GLuint cubeVAO, cubeVBO;

		//Shape Data (NOTE: release mode will be needed for numbers above 100)
		uint32_t numStartCubes = 1000;
		//uint32_t numStartCubes = 10000;
		//uint32_t numStartCubes = 50000; //NOTE: disable rendering D_cubes and grids (hold O and C)
		//std::vector<D_CollisionCubeEntity> D_cubes; //move ctors clear out unique ptrs to shape
		std::vector<D_CollisionCubeEntity> D_cubes;
		//std::vector<glm::vec3> gravityPoints = {
		//	glm::vec3(100, 0, 0),
		//	glm::vec3(-100,0,-100),
		//	glm::vec3(-100,0,100),
		//	glm::vec3(0,0,100),
		//	glm::vec3(0,0,-100),
		//};

		std::mt19937 rng_eng;
		float gravityDapeningFactor = 0.001f;

		//State
		CameraFPS camera{ 45.0f, -90.f, 0.f };
		ColumnBasedTransform lightTransform;
		std::shared_ptr<ColumnBasedTransform> transformTarget;
		glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };
		Shader objShader{ litObjectShaderVertSrc , litObjectShaderFragSrc, false };
		Shader lampShader{ lightLocationShaderVertSrc, lightLocationShaderFragSrc, false };
		Shader debugGridShader{ SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false };
		Shader fpsShader{ Utility::DigitalClockShader_Vertex, Utility::DigitalClockShader_Fragment, false };

		//Modifiable State
		float moveSpeed = 3;
		float rotationSpeed = 45.0f;
		bool bEnableCollision = false;
		bool bBlockCollisionFailures = true;
		bool bTickUnitTests = false;
		bool bUseCameraAxesForObjectMovement = true;
		bool bEnableAxisOffset = false;
		bool bRenderHashGrid = false;
		bool bRenderOccupiedCells = false;
		bool bRenderCubes = true;
		bool bUseSpatialHash = true;
		bool bUserOptimizedUpdate = true;

		glm::vec3 axisOffset{ 0, 0.005f, 0 };
		glm::vec3 cachedVelocity;

		SH::SpatialHashGrid<CollisionEntity> spatialHash{ glm::vec4{4,4,4,1} };

		Utility::FrameRateDisplay fpsDisplay;

	public:
		UniquePtrOddnessDemo(int width, int height)
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

			//for now, just have a dummy object because this demo may at one point want to manipulate some object
			transformTarget = std::make_shared<ColumnBasedTransform>();

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

			lightTransform.position = glm::vec3(5, 3, 0);
			lightTransform.scale = glm::vec3(0.1f);

			////////////////////////////////////////////////////////////
			// initialization state
			////////////////////////////////////////////////////////////
			std::random_device rng;
			std::seed_seq seed{ rng(), rng(), rng(), rng(), rng(), rng(), rng(), rng() };
			rng_eng = std::mt19937(seed);
			//std::uniform_int_distribution<std::mt19937::result_type> start_dist(-10, 10);
			std::uniform_real_distribution<float> startDist(-50.f, 50.f); //[a, b)
			std::uniform_real_distribution<float> gravityDist(-25.f, 25.f); //[a, b)
			std::uniform_real_distribution<float> distSpeed(0, 3); //[a, b)
			std::uniform_int_distribution<int> colorEnabled(0, 1); //[a,b]
			//D_cubes.reserve(numStartCubes);
			for (uint32_t cubeIdx = 0; cubeIdx < numStartCubes; ++cubeIdx)
			{
				glm::vec3 startPos(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));

				//give it a point to attract to
				glm::vec3 gravityPnt(gravityDist(rng_eng), gravityDist(rng_eng), gravityDist(rng_eng));

				//give it some initial velocity
				glm::vec3 startVelocity(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));
				startVelocity = startVelocity == glm::vec3(0.f) ? glm::vec3(1, 0, 0) : startVelocity;
				startVelocity = glm::normalize(startVelocity);
				float speed = glm::abs(distSpeed(rng_eng)) + 1.0f;
				startVelocity *= speed;

				//ad-hoc random color
				glm::vec3 startColor(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng)); //start_dist is fine, will normalize
				startColor.x = (bool)colorEnabled(rng_eng) ? startColor.x : 0.0f;
				startColor.y = (bool)colorEnabled(rng_eng) ? startColor.y : 0.0f;
				startColor.z = (bool)colorEnabled(rng_eng) ? startColor.z : 0.0f;
				if (startColor.x < 0.1f && startColor.y < 0.1f && startColor.z < 0.1f)
				{
					startColor = glm::vec3(startDist(rng_eng), startDist(rng_eng), 1.f);
				}
				startColor = glm::normalize(glm::abs(startColor));


				ColumnBasedTransform newTransform;
				newTransform.position = startPos;

				//D_cubes.push_back(newTransform);//seems to overwrite previous memory in a non-defined move ctor
				D_cubes.push_back(D_CollisionCubeEntity{ newTransform });
 				D_CollisionCubeEntity& newCube = D_cubes[D_cubes.size() - 1];
				newCube.color = startColor;
				newCube.velocity = startVelocity;
				newCube.gravityPnt = gravityPnt;
				if (bUseSpatialHash)
				{
					newCube.spatialHashEntry = spatialHash.insert(newCube, newCube.getOBB());
				}
			}

			//below shows the error:
			//spatial hash has a reference to the D_CollisionCubeEntity
			//when we move the entity, the reference in SpatialHashEntry
			//now points to the wrong object!
			D_CollisionCubeEntity testMove(lightTransform);
			testMove.spatialHashEntry = spatialHash.insert(testMove, testMove.getOBB());
			D_CollisionCubeEntity moved(std::move(testMove));
			std::cout << "test move done, break here" << std::endl;

			std::vector<D_CollisionCubeEntity> TestMoveVector;
			TestMoveVector.push_back(lightTransform);
			TestMoveVector[0].spatialHashEntry = spatialHash.insert(TestMoveVector[0], TestMoveVector[0].getOBB());
			TestMoveVector.push_back(lightTransform);
			std::cout << "test vector move done, break here" << std::endl;
		}

		~UniquePtrOddnessDemo()
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

			fpsDisplay.tick();

			processInput(window);

			std::vector<std::shared_ptr<SH::GridNode<CollisionEntity>>> overlapingNodes;
			overlapingNodes.reserve(10);

			for (D_CollisionCubeEntity& cube : D_cubes)
			{
				cube.transform.position += cube.velocity * deltaTime;
				if (bUseSpatialHash)
				{
					if (bUserOptimizedUpdate)
					{
						spatialHash.updateEntry(cube.spatialHashEntry, cube.getOBB());
					}
					else
					{
						cube.spatialHashEntry = spatialHash.insert(cube, cube.getOBB());
					}


					//this collision test is over-simplified. It just does a single collision pass. 
					//a more clever collision pass would probably take into account further collisions
					//with objects already tested if a collision occurs.
					if (bEnableCollision)
					{
						overlapingNodes.clear();
						spatialHash.lookupNodesInCells(*cube.spatialHashEntry, overlapingNodes);
						for (std::shared_ptr<SH::GridNode<CollisionEntity>>& node : overlapingNodes)
						{
							//don't do self collisions
							if (&node->element != &cube)
							{
								glm::vec4 mtv;
								if (SAT::Shape::CollisionTest(*cube.myShape, *node->element.myShape, mtv))
								{
									//correct collision
									cube.transform.position += glm::vec3(mtv);
									spatialHash.updateEntry(cube.spatialHashEntry, cube.getOBB()); //update spatial hash after collision

									//below is totally a adhoc physics lol

									//impact velocity to the moving cube 
									float cubeSpeed = glm::length(cube.velocity);
									glm::vec4 mtv_n = glm::normalize(mtv);
									cube.velocity = mtv_n * cubeSpeed;

									//impact have velocity to the object we collided 
									node->element.velocity += 0.5f * cube.velocity;
								}
							}
						}
					}
				}

				//adhoc gravitate towards points
				//glm::vec3 vecToCenter = (glm::vec3(0, 0, 0) - cube.transform.position);
				//cube.velocity += gravityDapeningFactor * vecToCenter;
				//for (glm::vec3& gravityPnt : gravityPoints)
				//{
				//	glm::vec3 vecToPnt = (gravityPnt - cube.transform.position);
				//	cube.velocity += gravityDapeningFactor * vecToPnt;
				//}				
				glm::vec3 vecToGravity = (cube.gravityPnt - cube.transform.position);
				cube.velocity += gravityDapeningFactor * vecToGravity;
			}

			mat4 view = camera.getView();
			mat4 projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(width) / height, 0.1f, 300.0f);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (bRenderHashGrid)
			{
				SH::drawAABBGrid<CollisionEntity>(spatialHash, glm::vec3(0.2f), debugGridShader, glm::mat4(1.0f), view, projection);
			}

			if (bRenderOccupiedCells)
			{
				glDepthFunc(GL_ALWAYS);
				for (D_CollisionCubeEntity& cube : D_cubes)
				{
					//this is going to be slow
					std::vector<std::shared_ptr<const SH::HashCell<CollisionEntity>>> cells;

					spatialHash.lookupCellsForEntry(*cube.spatialHashEntry, cells);
					std::vector<glm::ivec3> cellLocs;
					cellLocs.reserve(cells.size());
					for (const std::shared_ptr<const SH::HashCell<CollisionEntity>>& cell : cells)
					{
						cellLocs.push_back(cell->location);
					}
					SH::drawCells(cellLocs, spatialHash.gridCellSize, cube.color, debugGridShader, glm::mat4(1.0f), view, projection);
				}
				glDepthFunc(GL_LESS);
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

			//render D_cubes
			if (bRenderCubes)
			{
				for (D_CollisionCubeEntity& cube : D_cubes)
				{
					mat4 model = cube.transform.getModelMatrix();
					objShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
					objShader.setUniform3f("objectColor", cube.color);
					glDrawArrays(GL_TRIANGLES, 0, 36);
				}
			}


			fpsDisplay.render({ width, height }, fpsShader.getId());
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
				std::cout
					<< " Press P to print debug information" << std::endl
					<< " Press X to toggle spatial hash usage." << std::endl
					<< " Press 9/0 to decrease/increase scale" << std::endl
					<< " U to toggle efficent 'update' function instead of always re-inserting" << std::endl
					<< " X to toggle usage of spatial hashing " << std::endl
					<< " O to toggle rendering of D_cubes" << std::endl
					<< " C to toggle rendering spatial hash cells (necessary with large numbers of D_cubes)" << std::endl
					<< " F to toggle collision" << std::endl
					<< " see code for other toggles (function processInput)" << std::endl
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
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_V))
			{
				bUseCameraAxesForObjectMovement = !bUseCameraAxesForObjectMovement;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_M))
			{
				bEnableAxisOffset = !bEnableAxisOffset;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_F))
			{
				bEnableCollision = !bEnableCollision;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_H))
			{
				bRenderHashGrid = !bRenderHashGrid;
				bRenderOccupiedCells = !bRenderOccupiedCells;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_X))
			{
				bUseSpatialHash = !bUseSpatialHash;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_R))
			{
				bRenderHashGrid = !bRenderHashGrid;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_C))
			{
				bRenderOccupiedCells = !bRenderOccupiedCells;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_O))
			{
				bRenderCubes = !bRenderCubes;
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_U))
			{
				bUserOptimizedUpdate = !bUserOptimizedUpdate;
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

				spatialHash.logDebugInformation();
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_L))
			{
				//profiler entry point
				int profiler_helper = 5;
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

		UniquePtrOddnessDemo demo(width, height);
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
//	A419::true_main();
//}