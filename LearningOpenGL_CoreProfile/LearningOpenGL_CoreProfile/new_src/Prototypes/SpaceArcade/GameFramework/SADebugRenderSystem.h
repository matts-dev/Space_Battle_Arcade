#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <optional>
#include <set>

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

	struct shapeData
	{
		std::vector<glm::mat4> models;
		std::vector<glm::vec4> colors;

		void clear()
		{
			colors.clear();
			models.clear();
		}
	};

	struct FrameData_VisualDebugging
	{
		//line data is packaged shear matrices that transform the basis vectors into the end points of the line. third column is color.
		std::vector<glm::mat4> lineData;
		shapeData cubeData;
	};

	////////////////////////////////////////////////////////
	// Debug shape mesh
	////////////////////////////////////////////////////////
	class DebugShapeMesh : public ShapeMesh
	{
	public:
		void setProjectionViewMatrix(const glm::mat4& projection_view) { this->projection_view = projection_view; }
		GLuint getMat4InstanceVBO() { return vboInstancedMat4s; }
		GLuint getVec4InstanceVBO() { return vboInstancedVec4s; }

	protected:
		glm::mat4 projection_view;
		GLuint vboInstancedMat4s = 0;
		GLuint vboInstancedVec4s = 0;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// Handle rendering instanced lines.
	/////////////////////////////////////////////////////////////////////////////////////
	class DebugLineRender : public DebugShapeMesh
	{
	public:
		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;
		virtual const std::vector<unsigned int>& getVAOs() override;

	private:
		virtual void onReleaseOpenGLResources() override;
		virtual void onAcquireOpenGLResources() override;

	private:
		sp<Shader> shader = nullptr;
		std::vector<unsigned int> vaos;
		GLuint vao = 0;
		GLuint vboPositions = 0;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// handle rendering debug cubes
	/////////////////////////////////////////////////////////////////////////////////////
	class DebugCubeRender : public DebugShapeMesh
	{
	public:
		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;
		virtual const std::vector<unsigned int>& getVAOs() override;

	public:

	private:
		virtual void onReleaseOpenGLResources() override;
		virtual void onAcquireOpenGLResources() override;

	private:
		sp<Shader> shader = nullptr;
		std::vector<unsigned int> vaos;
		GLuint vao = 0;
		GLuint vboPositions = 0;
	};

	struct TimedRenderDatum;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Timed render structs (render for X seconds)
	//		Timer provides a per-object way of setting a "time up" flag
	//		Actual debug renderer will tick this objects when it renders
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct TimedRenderDatum : public GameEntity
	{
		float timerDurationSecs;
		bool bTimerUp = false;
		sp<MultiDelegate<>> timerDelegate = nullptr;

		TimedRenderDatum(float secs)
			: timerDurationSecs(secs)
		{
		}

		float getColorScaledown(float accumulatedTime) const;

		void HandleTimeUp()
		{
			bTimerUp = true;
		}
		void registerTimer();
		virtual void render(float dtSec, class DebugRenderSystem& debugRenderSys) = 0;
	};

	struct LineTimedRenderDatum : TimedRenderDatum
	{
		const glm::vec3 pntA;
		const glm::vec3 pntB;
		const glm::vec3 color;
		float accumulatedTime = 0.0f;

		LineTimedRenderDatum(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color, float secs)
			: TimedRenderDatum(secs),
			pntA(pntA),
			pntB(pntB),
			color(color)
		{
		}
		virtual void render(float dtSec, class DebugRenderSystem& debugRenderSys) override;
	};

	struct CubeTimedRenderDatum : TimedRenderDatum
	{
		const glm::mat4 model;
		const glm::vec3 color;
		float accumulatedTime = 0.0f;

		CubeTimedRenderDatum(const glm::mat4& model, const glm::vec3& color, float secs)
			: TimedRenderDatum(secs),
			model(model),
			color(color)
		{
		}
		virtual void render(float dtSec, class DebugRenderSystem& debugRenderSys) override;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug Render System - uses instanced rendering for efficiency.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class DebugRenderSystem : public SystemBase
	{
	public:
		void renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color);
		void renderLineOverTime(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color, float secs);

		void renderCube(const glm::mat4& model, const glm::vec3& color);
		void renderCubeOverTime(const glm::mat4& model, const glm::vec3& color, float secs);

	private:
		virtual void initSystem() override;
		virtual void handleRenderDispatch(float dt_sec_system);
		virtual void handleFrameOver(uint64_t endingFrameNumber);
		void writeTimedDataToFrame();

	private:
		FrameShuffler<FrameData_VisualDebugging> frameSwitcher{ 1 };
		sp<DebugLineRender> lineRenderer;
		sp<DebugCubeRender> cubeRenderer;
		std::set<sp<TimedRenderDatum>> timedRenderData;
	};

}