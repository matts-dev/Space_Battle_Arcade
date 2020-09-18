#pragma once
#include <vector>
#include "SASystemBase.h"
#include "../Tools/Algorithms/AmortizeLoopTool.h"
#include "../Tools/DataStructures/SATransform.h"

namespace SA
{
	struct RenderData;
	class GameBase;

	class DeferredRendererStateMachine;
	class ForwardRenderingStateMachine;
	class PointLight_Deferred;
	class Shader;

	//deferred rendering refactor is not finished, this is used to disable todos that are preventing compilation; flip to 0 to get easily to read compilation errors on work left to be done
#define IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE 1

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The encapsulated render system
	//
	// #todo #nextengine perhaps render system should be more first class rather than have GameBase controling things
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class RenderSystem final : public SystemBase
	{
	public:
		/** Subclasses of GameBase can write to a frames data, whereas everyone else can only read from the data */
		const RenderData* getFrameRenderData_Read(uint64_t frameNumber)														{ return getFrameRenderData(frameNumber); }
		RenderData*		  getFrameRenderData_Write(uint64_t frameNumber, const struct GamebaseIdentityKey& privateKey)		{ return getFrameRenderData(frameNumber); }

		void enableDeferredRenderer(bool bEnable);
		bool usingDeferredRenderer() { return deferredRenderer != nullptr; }
		DeferredRendererStateMachine* getDeferredRenderer(){ return deferredRenderer.get(); };
		ForwardRenderingStateMachine* getForwardRenderer() { return forwardRenderer.get(); }
		const std::vector<sp<PointLight_Deferred>>& getFramePointLights() { return userPointLights; }
		const sp<PointLight_Deferred> createPointLight();
		bool isUsingHDR();
	protected:
		virtual void tick(float dt_sec) override;;
	private:
		/** Private to allow deciding whether to return this as constant data or mutable data.*/
		RenderData* getFrameRenderData(uint64_t frameNumber);
	private:
		virtual void initSystem() override;
	private:
		AmortizeLoopTool amort_PointLight_GC;
		std::vector<sp<RenderData>> renderFrameCircularBuffer;
		std::vector<sp<PointLight_Deferred>> userPointLights; //#todo if end up saving previous frame data (ie the circular buffer) then these need their state saved each frame (ie just copy struct into RenderData struct)
		sp<DeferredRendererStateMachine> deferredRenderer = nullptr;
		sp<ForwardRenderingStateMachine> forwardRenderer = nullptr;
	};
}