#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include "../../GettingStarted/Camera/CameraFPS.h"
#include "../../../InputTracker.h"
#include "../../nu_utils.h"
#include "../../../Shader.h"
#include "SATComponent.h"
#include "SATUnitTestUtils.h"

namespace
{
	using SAT::ColumnBasedTransform;

	CameraFPS camera(45.0f, -90.f, 0.f);

	float lastFrameTime = 0.f;
	float deltaTime = 0.0f;

	const char* Dim2VertShaderSrc = R"(
				#version 330 core
				layout (location = 0) in vec2 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					vec4 pos = vec4(position, 0, 1);

					gl_Position = projection * view * model * pos;
				}
			)";
	const char* Dim2FragShaderSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

	const char* Dim3DebugSShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec4 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * position;
				}
			)";
	const char* Dim3DebugSShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

	void drawDebugLine(
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection
	)
	{
		/* This method is extremely slow and non-performant; use only for debugging purposes */
		static Shader shader(Dim3DebugSShaderVertSrc, Dim3DebugSShaderFragSrc, false);

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

	ColumnBasedTransform boxTransform;
	ColumnBasedTransform triTransform;
	ColumnBasedTransform* transformTarget = &triTransform;
	float moveSpeed = 200;
	float rotationSpeed = 45.0f;
	glm::vec3 cachedVelocity;
	bool bEnableCollision = true;
	bool bBlockCollisionFailures = false;
	bool bTickUnitTests = false;
	SATShape2D* gBoxCollision = nullptr;
	SATShape2D* gTriCollision = nullptr;
	std::shared_ptr<SAT::TestSuite> UnitTests;

	void printVec3(glm::vec3 v)
	{
		std::cout << "[" << v.x << ", " << v.y << ", " << v.z << "]";
	}

	void processInput(GLFWwindow* window)
	{
		using glm::vec3; using glm::vec4;
		static InputTracker input; //using static vars in polling function may be a bad idea since cpp11 guarantees access is atomic -- I should bench this
		input.updateState(window);

		if (input.isKeyJustPressed(window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(window, true);
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_T))
		{
			transformTarget = transformTarget == &triTransform ? &boxTransform : &triTransform;
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_P))
		{
			
		}
		if (input.isKeyJustPressed(window, GLFW_KEY_U))
		{
			bTickUnitTests = !bTickUnitTests;
			if (bTickUnitTests)
			{
				UnitTests->restartAllTests();
				UnitTests->start();
			}
		}

		// -------- MOVEMENT -----------------
		bool bUpdateMovement = false;
		vec3 movementInput = vec3(0.0f);
		vec3 cachedTargetPos = transformTarget->position;
		if (input.isKeyDown(window, GLFW_KEY_W))
		{
			movementInput += vec3(0, 1, 0);
		}
		if (input.isKeyDown(window, GLFW_KEY_S))
		{
			movementInput += vec3(0, -1, 0);
		}
		if (input.isKeyDown(window, GLFW_KEY_A))
		{
			if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
				transformTarget->rotationDeg.z += -rotationSpeed * deltaTime;
			else
				movementInput += vec3(-1, 0, 0);
		}
		if (input.isKeyDown(window, GLFW_KEY_D))
		{
			if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL) || input.isKeyDown(window, GLFW_KEY_RIGHT_CONTROL))
				transformTarget->rotationDeg.z += rotationSpeed * deltaTime;
			else
				movementInput += vec3(1, 0, 0);
		}
		//probably should use epsilon comparison for when opposite buttons held, but this should cover compares against initialization
		if (movementInput != vec3(0.0f))
		{
			movementInput = glm::normalize(movementInput);
			cachedVelocity = deltaTime * moveSpeed * movementInput;
			transformTarget->position += cachedVelocity;
		}
		if (bBlockCollisionFailures)
		{
			if (SATShape::CollisionTest(*gTriCollision, *gBoxCollision))
			{
				//undo the move
				transformTarget->position = cachedTargetPos;
			}
		}
		// -------- UNIT TEST ---------
		if (input.isKeyJustPressed(window, GLFW_KEY_UP))
		{
		}
		else if (input.isKeyJustPressed(window, GLFW_KEY_DOWN))
		{
			UnitTests->restartCurrentTest();
		}
		else if (input.isKeyJustPressed(window, GLFW_KEY_LEFT))
		{
			UnitTests->previousTest();
		}
		else if (input.isKeyJustPressed(window, GLFW_KEY_RIGHT))
		{
			UnitTests->nextTest();
		}


		camera.handleInput(window, deltaTime);
	}


	void true_main()
	{
		using glm::vec2;
		using glm::vec3;
		using glm::mat4;

		camera.setPosition(0.0f, 0.0f, 3.0f);
		int width = 1200;
		int height = 800;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		camera.exclusiveGLFWCallbackRegister(window);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		/////////////////////////////////////////////////////////////////////
		// Object Creation
		/////////////////////////////////////////////////////////////////////

		//rectangle
		std::vector<vec2> boxPnts = { vec2(0.5f, 0.5f), vec2(-0.5f, 0.5f), vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f) };
		GLuint box2dVAO, box2dVBO, box2dEAO;
		glGenVertexArrays(1, &box2dVAO);
		glBindVertexArray(box2dVAO);
		float box2d_verts[] = {
			boxPnts[0].x, boxPnts[0].y, //0.5f, 0.5f,
			boxPnts[1].x, boxPnts[1].y, //-0.5f, 0.5f,
			boxPnts[2].x, boxPnts[2].y, //-0.5f, -0.5f,
			boxPnts[3].x, boxPnts[3].y, //0.5f, -0.5f
		};
		glGenBuffers(1, &box2dVBO);
		glBindBuffer(GL_ARRAY_BUFFER, box2dVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(box2d_verts), box2d_verts, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		GLuint box2d_idxs[] = {
			0,1,2,
			2,3,0
		};
		glGenBuffers(1, &box2dEAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, box2dEAO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(box2d_idxs), box2d_idxs, GL_STATIC_DRAW);
		glBindVertexArray(0); //unbind VAO so no further state is saved
		boxTransform = { vec3(0, 0, 0), vec3(0,0,0), vec3(100, 100, 100) };
		SATShape2D boxCollision{ SATShape2D::ConstructHelper(boxPnts) };

		//triangle
		std::vector<vec2> triPnts = { vec2(0.0f, 0.5f), vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f) };
		GLuint tri2dVAO, tri2dVBO;
		glGenVertexArrays(1, &tri2dVAO);
		glBindVertexArray(tri2dVAO);
		float tri2d_verts[] = {
			triPnts[0].x, triPnts[0].y,
			triPnts[1].x, triPnts[1].y,
			triPnts[2].x, triPnts[2].y
		};
		glGenBuffers(1, &tri2dVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tri2dVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tri2d_verts), tri2d_verts, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glBindVertexArray(0); //unbind VAO so no further state is saved
		triTransform = { vec3(-width / 4, -height / 4, 0), vec3(0,0,0), vec3(100, 100, 100) };
		SATShape2D triCollision{ SATShape2D::ConstructHelper(triPnts) };

		gBoxCollision = &boxCollision;
		gTriCollision = &triCollision;

		Shader shader2d(Dim2VertShaderSrc, Dim2FragShaderSrc, false);

		/////////////////////////////////////////////////////////////////////
		// Unit Test Creation
		/////////////////////////////////////////////////////////////////////

		UnitTests = std::make_shared<SAT::TestSuite>();

		// Unit Test testing 4 cardinal 2d directions


		/////////////////////////////////////////////////////////////////////
		// Game Loop
		/////////////////////////////////////////////////////////////////////
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			const vec3 boxColor(0, 0, 1);
			const vec3 triColor(1, 0, 0);

			//collision should be adjusted and complete before we attempt to render anything
			triCollision.updateTransform(triTransform.getModelMatrix());
			boxCollision.updateTransform(boxTransform.getModelMatrix());
			if (bEnableCollision && !UnitTests->isRunning())
			{
				glm::vec4 mtv;
				if (SATShape::CollisionTest(triCollision, boxCollision, mtv))
				{
					std::cout << "MTV: "; printVec3(mtv); std::cout << "\tvel: "; printVec3(cachedVelocity); std::cout << "\tpos:"; printVec3(triTransform.position); std::cout << std::endl;
					triTransform.position += vec3(mtv);
					triCollision.updateTransform(triTransform.getModelMatrix());
				}
				else
				{
					if (cachedVelocity != vec3(0.0f))
					{
						std::cout << "\t\t\t"; std::cout << "\tvel: "; printVec3(cachedVelocity); std::cout << "\tpos:"; printVec3(triTransform.position); std::cout << std::endl;
						cachedVelocity = vec3(0.0f);
					}
				}
			}
			else if (bTickUnitTests)
			{
				UnitTests->tick(deltaTime);
			}

			mat4 view = glm::lookAt(vec3(0, 0, 5.0f), vec3(0, 0, 0), vec3(0, 1.0f, 0));
			mat4 projection = glm::ortho(-(float)width / 2.0f, (float)width / 2.0f, -(float)height / 2.0f, (float)height / 2.0f, 0.1f, 100.0f);
			shader2d.use();
			shader2d.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			shader2d.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			{ //draw box
				//resist temptation to update collision transform here, collision should be separated from rendering (for mtv corrections)
				mat4 model = boxTransform.getModelMatrix();
				shader2d.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader2d.setUniform3f("color", boxColor);
				glBindVertexArray(box2dVAO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, box2dEAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			{// draw triangle
				//resist temptation to update collision transform here, collision should be separated from rendering (for mtv corrections)
				mat4 model = triTransform.getModelMatrix();
				shader2d.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				shader2d.setUniform3f("color", triColor);
				glBindVertexArray(tri2dVAO);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}

			//draw collision axes
			std::vector<glm::vec3> boxAxes, triAxes;
			boxCollision.appendFaceAxes(boxAxes);
			triCollision.appendFaceAxes(triAxes);
			constexpr float axisSizeScale = 1000.0f;

			//intentionally draw axes before projections on axes
			for (const glm::vec3& axis : boxAxes)
			{
				drawDebugLine(vec3(0.0f), axis * axisSizeScale, vec3(boxColor / 2.0f), mat4(1.0f), view, projection);
				drawDebugLine(vec3(0.0f), -axis * axisSizeScale, vec3(boxColor / 2.0f), mat4(1.0f), view, projection);
			}
			for (const glm::vec3& axis : triAxes)
			{
				drawDebugLine(vec3(0.0f), axis * axisSizeScale, vec3(triColor / 2.0f), mat4(1.0f), view, projection);
				drawDebugLine(vec3(0.0f), -axis * axisSizeScale, vec3(triColor / 2.0f), mat4(1.0f), view, projection);
			}
			std::vector<glm::vec3> allAxes = boxAxes;
			allAxes.insert(allAxes.end(), triAxes.begin(), triAxes.end());
			for (const glm::vec3& axis : allAxes)
			{
				SAT::ProjectionRange projection1 = boxCollision.projectToAxis(axis);
				vec3 pnt1A = axis * projection1.min;
				vec3 pnt1B = axis * projection1.max;

				SAT::ProjectionRange projection2 = triCollision.projectToAxis(axis);
				vec3 pnt2A = axis * projection2.min;
				vec3 pnt2B = axis * projection2.max;

				bool bDisjoint = projection1.max < projection2.min || projection2.max < projection1.min;
				vec3 overlapColor = boxColor + triColor;
				drawDebugLine(pnt1A, pnt1B, bDisjoint ? boxColor * 0.75f : overlapColor, mat4(1.0f), view, projection);
				drawDebugLine(pnt2A, pnt2B, bDisjoint ? triColor * 0.75f : overlapColor, mat4(1.0f), view, projection);
			}


			glfwSwapBuffers(window);
			glfwPollEvents();

		}

		glDeleteVertexArrays(1, &box2dVAO);
		glDeleteBuffers(1, &box2dVBO);
		glDeleteVertexArrays(1, &tri2dVAO);
		glDeleteBuffers(1, &tri2dVBO);
		glfwTerminate();
	}
}

int main()
{
	true_main();
}