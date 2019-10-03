#include "SADebugRenderSystem.h"
#include "SAGameBase.h"
#include "SAPlayerBase.h"
#include "SAPlayerSystem.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/SAShader.h"

namespace SA
{
	void DebugRenderSystem::initSystem()
	{
		lineRenderer = new_sp<DebugLineRender>();

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
				ec(glBindBuffer(GL_ARRAY_BUFFER, lineRenderer->getMat4InstanceVBO()));
				ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * readFrame->lineData.size(), &readFrame->lineData[0], GL_STATIC_DRAW)); 

				GLuint lineVAO = lineRenderer->getVAOs()[0];
				ec(glBindVertexArray(lineVAO));

				ec(glBindBuffer(GL_ARRAY_BUFFER, lineRenderer->getMat4InstanceVBO()));
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
		}
	}

	void DebugRenderSystem::handleFrameOver(uint64_t endingFrameNumber)
	{
		//this needs to happen at a point that all threads are finished and waiting to be assigned more work.
		frameSwitcher.advanceFrame();
	}

	void DebugRenderSystem::renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color)
	{
		using namespace glm;
		//use a shear matrix trick to package all this data into a single mat4.
		// shear matrix trick transforms basis vectors into the columns of the provided matrix.
		mat4 shearMatrix = mat4(vec4(pntA,0), vec4(pntB,0), vec4(color,1), vec4(0.f, 0.f, 0.f, 1.f));


		frameSwitcher.getWriteFrame().lineData.push_back(shearMatrix);
	}

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

}
