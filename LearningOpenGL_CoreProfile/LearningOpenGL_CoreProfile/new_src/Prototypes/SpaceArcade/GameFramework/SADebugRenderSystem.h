#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <optional>

#include "SASystemBase.h"
#include "SAGameEntity.h"
#include "../Tools/Geometry/SimpleShapes.h"

namespace SA
{
	class Shader;



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Structured frame management.
	//
	// Allows loading of data into a frame while reading from another frame.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename FRAME_DATA_TYPE>
	class FrameShuffler
	{
	public:
		FrameShuffler(uint32_t numFrames)
		{
			frames.resize(numFrames);
		}

		uint32_t advanceFrame()
		{
			lastFrame = currentFrame;
			currentFrame = (currentFrame + 1) % frames.size();

			return currentFrame;
		}

		inline FRAME_DATA_TYPE& getWriteFrame() { return frames[currentFrame]; }
		inline FRAME_DATA_TYPE* getReadFrame()
		{
			if (lastFrame.has_value())
			{
				return &frames[*lastFrame];
			}
			return nullptr;
		}

	private:
		uint32_t currentFrame = 0;
		std::optional<uint32_t> lastFrame;
		std::vector<FRAME_DATA_TYPE> frames;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Utility to provide and release indices in an array
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//class FreeIndexManager
	//{
	//public:
	//	size_t claimIndex()
	//	{
	//		if (releasedIndices.size() > 0)
	//		{
	//			size_t reclaimedIndex = releasedIndices.back();
	//			releasedIndices.pop_back();
	//			return reclaimedIndex;
	//		}
	//		return nextSequentialIndex++;
	//	}
	//	void releaseIndex(size_t index)
	//	{
	//		releasedIndices.push_back(index);
	//	}
	//private:
	//	std::vector<size_t> releasedIndices;
	//	size_t nextSequentialIndex = 0;
	//};

	struct FrameData_VisualDebugging
	{
		//line data is packaged shear matrices that transform the basis vectors into the end points of the line. third column is color.
		std::vector<glm::mat4> lineData;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Handle rendering instanced lines.
	/////////////////////////////////////////////////////////////////////////////////////
	class DebugLineRender : public ShapeMesh
	{
	public:
		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;
		virtual const std::vector<unsigned int>& getVAOs() override;

	public:
		GLuint getMat4InstanceVBO() { return vboInstancedMat4s; }
		void setProjectionViewMatrix(const glm::mat4& projection_view) { this->projection_view = projection_view; }

	private:
		virtual void onReleaseOpenGLResources() override;
		virtual void onAcquireOpenGLResources() override;

	private:
		sp<Shader> shader = nullptr;
		glm::mat4 projection_view;
		std::vector<unsigned int> vaos;
		GLuint vao = 0;
		GLuint vboPositions = 0;
		GLuint vboInstancedMat4s = 0;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug Render System - uses instanced rendering for efficiency.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class DebugRenderSystem : public SystemBase
	{
	public:
		void renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color);

	private:
		virtual void initSystem() override;
		virtual void handleRenderDispatch(float dt_sec_system);
		virtual void handleFrameOver(uint64_t endingFrameNumber);

	private:
		FrameShuffler<FrameData_VisualDebugging> frameSwitcher{ 1 };
		sp<DebugLineRender> lineRenderer;
	};

}