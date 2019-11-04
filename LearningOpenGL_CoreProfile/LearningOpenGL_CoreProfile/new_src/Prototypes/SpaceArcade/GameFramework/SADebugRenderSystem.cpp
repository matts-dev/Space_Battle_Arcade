#include "SADebugRenderSystem.h"
#include "SAGameBase.h"
#include "SAPlayerBase.h"
#include "SAPlayerSystem.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/SAShader.h"
#include "SALevel.h"
#include "SALevelSystem.h"
#include <detail/func_common.hpp>

namespace SA
{
	void DebugRenderSystem::initSystem()
	{
		lineRenderer = new_sp<DebugLineRender>();
		cubeRenderer = new_sp<DebugCubeRender>();

		GameBase& gameBase = GameBase::get();
		gameBase.onRenderDispatch.addWeakObj(sp_this(), &DebugRenderSystem::handleRenderDispatch);
		gameBase.onFrameOver.addWeakObj(sp_this(), &DebugRenderSystem::handleFrameOver);
	}

	void DebugRenderSystem::handleRenderDispatch(float dt_sec_system)
	{
		if (FrameData_VisualDebugging* readFrame = frameSwitcher.getReadFrame())
		{
			// #TODO #threading below will need updating for a deferred frame system; we're going to need to cache player information at end of each frame.
			static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
			const sp<PlayerBase>& player = playerSystem.getPlayer(0);
			const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); 

			if (readFrame->lineData.size() > 0)
			{

				GLuint lineVAO = lineRenderer->getVAOs()[0];
				ec(glBindVertexArray(lineVAO));

				ec(glBindBuffer(GL_ARRAY_BUFFER, lineRenderer->getMat4InstanceVBO()));
				ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * readFrame->lineData.size(), &readFrame->lineData[0], GL_STATIC_DRAW)); 

				GLsizei numVec4AttribsInBuffer = 4;
				size_t packagedVec4Idx_matbuffer = 0;

				//load shear packed matrix
				ec(glEnableVertexAttribArray(1));
				ec(glEnableVertexAttribArray(2));
				ec(glEnableVertexAttribArray(3));
				ec(glEnableVertexAttribArray(4));

				ec(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));

				ec(glVertexAttribDivisor(1, 1));
				ec(glVertexAttribDivisor(2, 1));
				ec(glVertexAttribDivisor(3, 1));
				ec(glVertexAttribDivisor(4, 1));

				lineRenderer->setProjectionViewMatrix(camera->getPerspective() * camera->getView());
				lineRenderer->instanceRender(readFrame->lineData.size());
			}
			readFrame->lineData.clear();

			if (readFrame->cubeData.models.size() > 0)
			{
				GLuint cubeVAO = cubeRenderer->getVAOs()[0];
				ec(glBindVertexArray(cubeVAO));

				ec(glBindBuffer(GL_ARRAY_BUFFER, cubeRenderer->getMat4InstanceVBO()));
				ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * readFrame->cubeData.models.size(), &readFrame->cubeData.models[0], GL_STATIC_DRAW));
				GLsizei numVec4AttribsInBuffer = 4;
				size_t packagedVec4Idx_matbuffer = 0;

				//load model matrix
				ec(glEnableVertexAttribArray(1));
				ec(glEnableVertexAttribArray(2));
				ec(glEnableVertexAttribArray(3));
				ec(glEnableVertexAttribArray(4));
				ec(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
				ec(glVertexAttribDivisor(1, 1));
				ec(glVertexAttribDivisor(2, 1));
				ec(glVertexAttribDivisor(3, 1));
				ec(glVertexAttribDivisor(4, 1));

				//load color
				ec(glBindBuffer(GL_ARRAY_BUFFER, cubeRenderer->getVec4InstanceVBO()));
				ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * readFrame->cubeData.colors.size(), &readFrame->cubeData.colors[0], GL_STATIC_DRAW));

				ec(glEnableVertexAttribArray(5));
				ec(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), reinterpret_cast<void*>(0)));
				ec(glVertexAttribDivisor(5, 1));
				cubeRenderer->setProjectionViewMatrix(camera->getPerspective() * camera->getView());
				cubeRenderer->instanceRender(readFrame->cubeData.models.size());
			}
			readFrame->cubeData.clear();

		}
	}

	void DebugRenderSystem::handleFrameOver(uint64_t endingFrameNumber)
	{
		//before advancing frame write out the timed render data to the frame
		writeTimedDataToFrame();


		//this needs to happen at a point that all threads are finished and waiting to be assigned more work.
		frameSwitcher.advanceFrame();
	}

	void DebugRenderSystem::writeTimedDataToFrame()
	{
		static std::vector<sp<TimedRenderDatum>> removeData;
		static int removeDataInit = [](std::vector<sp<TimedRenderDatum>>& removeData)->int{ removeData.reserve(10); return 0; }(removeData);

		static LevelSystem& levelSys = GameBase::get().getLevelSystem();
		if (const sp<LevelBase>& currentLevel = levelSys.getCurrentLevel())
		{
			float dt_sec = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();
			for (const sp<TimedRenderDatum>& timedData : timedRenderData)
			{
				timedData->render(dt_sec, *this);
				if (timedData->bTimerUp)
				{
					removeData.push_back(timedData);
				}
			}
		}

		for (const sp<TimedRenderDatum>& renderDatum : removeData)
		{
			timedRenderData.erase(renderDatum);
		}
		removeData.clear();
	}

	void DebugRenderSystem::renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color)
	{
		using namespace glm;
		//use a shear matrix trick to package all this data into a single mat4.
		// shear matrix trick transforms basis vectors into the columns of the provided matrix.
		mat4 shearMatrix = mat4(vec4(pntA,0), vec4(pntB,0), vec4(color,1), vec4(0.f, 0.f, 0.f, 1.f));

		frameSwitcher.getWriteFrame().lineData.push_back(shearMatrix);
	}

	void DebugRenderSystem::renderLineOverTime(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color, float secs)
	{
		sp<TimedRenderDatum> lineData = new_sp<LineTimedRenderDatum>(pntA, pntB, color, secs);
		lineData->registerTimer();
		timedRenderData.insert(lineData);
	}

	void DebugRenderSystem::renderCube(const glm::mat4& model, const glm::vec3& color)
	{
		FrameData_VisualDebugging& writeFrame = frameSwitcher.getWriteFrame();
		writeFrame.cubeData.models.push_back(model);
		writeFrame.cubeData.colors.push_back(glm::vec4(color, 1.f));
	}

	void DebugRenderSystem::renderCubeOverTime(const glm::mat4& model, const glm::vec3& color, float secs)
	{
		sp<TimedRenderDatum> cubeData = new_sp<CubeTimedRenderDatum>(model, color, secs);
		cubeData->registerTimer();
		timedRenderData.insert(cubeData);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug line renderer
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DebugLineRender::render() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DebugLineRender::instanceRender(int instanceCount) const
	{
		if (shader)
		{
			shader->use();
			shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));

			ec(glBindVertexArray(vao));
			ec(glDrawArraysInstanced(GL_LINES, 0, 2, instanceCount));
		}
	}

	const std::vector<unsigned int>& DebugLineRender::getVAOs()
	{
		return vaos;
	}

	void DebugLineRender::onReleaseOpenGLResources()
	{
		vaos.clear();
		ec(glDeleteVertexArrays(1, &vao));
		ec(glDeleteBuffers(1, &vboPositions));
		ec(glDeleteBuffers(1, &vboInstancedMat4s));
		shader = nullptr;
	}

	void DebugLineRender::onAcquireOpenGLResources()
	{
		using namespace glm;

		const char* const line_vert_src = R"(
			#version 330 core

			layout (location = 0) in vec3 basis_vector;
			layout (location = 1) in mat4 shearMat;			//consumes locations 1, 2, 3, 4
			out vec4 color;

			uniform mat4 projection_view;

			void main()
			{
				gl_Position = projection_view * shearMat * vec4(basis_vector, 1);
				color = shearMat[2]; //3rd col is packed color
			}
		)";
		const char* const line_frag_src = R"(
			#version 330 core

			in vec4 color;
			out vec4 fragColor;

			void main()
			{
				fragColor = color;
			}
		)";

		shader = new_sp<Shader>(line_vert_src, line_frag_src, false); 

		ec(glBindVertexArray(0));
		ec(glGenVertexArrays(1, &vao));
		ec(glBindVertexArray(vao));
		vaos.push_back(vao);

		std::vector<vec3> vertPositions =
		{
			vec3(1,0,0),
			vec3(0,1,0)
		};
		ec(glGenBuffers(1, &vboPositions));
		ec(glBindBuffer(GL_ARRAY_BUFFER, vboPositions));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * vertPositions.size(), vertPositions.data(), GL_STATIC_DRAW));
		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));

		ec(glGenBuffers(1, &vboInstancedMat4s));

		//prevent other calls from corrupting this VAO state
		ec(glBindVertexArray(0));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug cube renderer
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DebugCubeRender::render() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


	void DebugCubeRender::instanceRender(int instanceCount) const
	{
		if (shader)
		{
			shader->use();
			shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));

			ec(glBindVertexArray(vao));

			ec(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
			ec(glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount));
			ec(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		}
	}

	const std::vector<unsigned int>& DebugCubeRender::getVAOs()
	{
		return vaos;
	}

	void DebugCubeRender::onReleaseOpenGLResources()
	{
		shader = nullptr;
		ec(glDeleteBuffers(1, &vboPositions));
		ec(glDeleteVertexArrays(1, &vao));
		ec(glDeleteBuffers(1, &vboInstancedMat4s));
		ec(glDeleteBuffers(1, &vboInstancedVec4s));
	}

	void DebugCubeRender::onAcquireOpenGLResources()
	{
		const float vertices[] = {
			//x      y      z   
			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f, 
			0.5f,  0.5f, -0.5f, 
			0.5f,  0.5f, -0.5f, 
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f, -0.5f,  0.5f,
			0.5f, -0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,

			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,

			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f, -0.5f, 
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 

			-0.5f, -0.5f, -0.5f,
			0.5f, -0.5f, -0.5f, 
			0.5f, -0.5f,  0.5f, 
			0.5f, -0.5f,  0.5f, 
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,

			-0.5f,  0.5f, -0.5f,
			0.5f,  0.5f, -0.5f, 
			0.5f,  0.5f,  0.5f, 
			0.5f,  0.5f,  0.5f, 
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
		};

		ec(glGenVertexArrays(1, &vao));
		ec(glBindVertexArray(vao));
		
		ec(glGenBuffers(1, &vboPositions));
		ec(glBindBuffer(GL_ARRAY_BUFFER, vboPositions));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

		ec(glGenBuffers(1, &vboInstancedMat4s));
		ec(glGenBuffers(1, &vboInstancedVec4s));

		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));

		ec(glBindVertexArray(0)); 

		vaos.clear();
		vaos.push_back(vao);


		const char* const cube_vert_src = R"(
			#version 330 core

			layout (location = 0) in vec3 position;
			layout (location = 1) in mat4 instanceModel;			//consumes locations 1, 2, 3, 4
			layout (location = 5) in vec4 instanceColor;			

			out vec4 color;

			uniform mat4 projection_view;

			void main()
			{
				color = instanceColor;
				gl_Position = projection_view * instanceModel * vec4(position, 1);
			}
		)";
		const char* const cube_frag_src = R"(
			#version 330 core

			in vec4 color;
			out vec4 fragColor;

			void main()
			{
				fragColor = color;
			}
		)";

		shader = new_sp<Shader>(cube_vert_src, cube_frag_src, false);
	}

	float TimedRenderDatum::getColorScaledown(float accumulatedTime) const
	{
		//add some factor so doesn't fade to black but just some lesser color
		float timeFactor = (1 - (accumulatedTime / timerDurationSecs)) + 0.2f; 
		return glm::clamp<float>(timeFactor, 0, 1.f);
	}

	////////////////////////////////////////////////////////
	// Timed render structs
	////////////////////////////////////////////////////////
	void TimedRenderDatum::registerTimer()
{
		timerDelegate = new_sp<MultiDelegate<>>();
		timerDelegate->addWeakObj(sp_this(), &TimedRenderDatum::HandleTimeUp);

		static LevelSystem& levelSys = GameBase::get().getLevelSystem();
		if (const sp<LevelBase>& level = levelSys.getCurrentLevel())
		{
			level->getWorldTimeManager()->createTimer(timerDelegate, timerDurationSecs);
		}
	}

	void LineTimedRenderDatum::render(float dtSec, DebugRenderSystem& debugRenderSys)
	{
		accumulatedTime += dtSec;

		//linearly fade color out over time
		float timeFactor = getColorScaledown(accumulatedTime);
		
		debugRenderSys.renderLine(pntA, pntB, color * timeFactor);
	}


	void CubeTimedRenderDatum::render(float dtSec, class DebugRenderSystem& debugRenderSys)
	{
		accumulatedTime += dtSec;

		//linearly fade color out over time
		float timeFactor = getColorScaledown(accumulatedTime);

		debugRenderSys.renderCube(model, color * timeFactor);
	}

}
