#include "SARenderSystem.h"
#include "SAGameBase.h"
#include "../Rendering/RenderData.h"
#include "../Rendering/SAGPUResource.h"
#include <algorithm>
#include "../Tools/SAUtilities.h"
#include "../Rendering/DeferredRendering/DeferredRendererStateMachine.h"
#include "../Rendering/SAShader.h"
#include "../Rendering/Lights/PointLight_Deferred.h"
#include "../Rendering/ForwardRendering/ForwardRenderingStateMachine.h"

namespace SA
{
	void RenderSystem::initSystem()
	{
		const EngineConstants& constants = GameBase::getConstants();
		renderFrameCircularBuffer.reserve(constants.RENDER_DELAY_FRAMES);

		//+1 because we need a 0 frame data
		for (uint64_t frameDataIdx = 0; frameDataIdx < (constants.RENDER_DELAY_FRAMES + 1); ++frameDataIdx)
		{
			renderFrameCircularBuffer.push_back(new_sp<RenderData>());
		}

		forwardRenderer = new_sp<ForwardRenderingStateMachine>();

		amort_PointLight_GC.chunkSize = 10;
	}

	RenderData* RenderSystem::getFrameRenderData(uint64_t frameNumber)
	{
		static GameBase& game = GameBase::get();
		assert(frameNumber == game.getFrameNumber()); //do not yet support frame delayed rendering

		//temporary, just return first item
		return renderFrameCircularBuffer[0].get();

		//#TODO #frame_delayed_rendering
		//static uint64_t delayFrames = GameBase::getConstants().RENDER_DELAY_FRAMES;
		//int8_t deltaFrame = int8_t(game.getFrameNumber() - frameNumber);
		//if (deltaFrame <= delayFrames)
		//{
		//	TODO calculate wrap around or use some datastructure with wrap around built in; preferable the later? 
		//}
		//return nullptr;

		//#TODO #frame_delayed_rendering note also, we're going to have to build a few frames data before rendering. Not sure where this will go.
	}

	void RenderSystem::enableDeferredRenderer(bool bEnable)
	{
		if (bEnable)
		{
			if (!deferredRenderer)
			{
				deferredRenderer = new_sp<DeferredRendererStateMachine>();
			}
		}
		else
		{
			if (deferredRenderer)
			{
				deferredRenderer->cleanup();
				deferredRenderer = nullptr;
			}
		}

	}

	void RenderSystem::tick(float dt_sec)
	{
		//SystemBase::tick(dt_sec);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// pseudo garbage collection (doesn't clean circular refernences)
		//
		// right now just doing this because it is a simple way to solve the problem of letting users create pointlights 
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		{
			static std::vector<size_t> pointLight_gcIndices;
			pointLight_gcIndices.reserve(amort_PointLight_GC.chunkSize);
			pointLight_gcIndices.clear();

			for (size_t idx = amort_PointLight_GC.updateStart(userPointLights);
				idx < amort_PointLight_GC.getStopIdxSafe(userPointLights);
				++idx)
			{
				const sp<PointLight_Deferred>& pointLight = userPointLights[idx];
			
				//determine if we should clear out the point light
				if (!pointLight)
				{
					pointLight_gcIndices.push_back(idx);
				}
				else
				{
					if (pointLight.use_count() == 1)
					{
						pointLight_gcIndices.push_back(idx);
					}
				}
			}

			//must process removal indices in reverse order so that we don't invalidate other indices
			std::sort(pointLight_gcIndices.begin(), pointLight_gcIndices.end(), std::greater<>());
			for (size_t idx : pointLight_gcIndices)
			{
				Utils::swapAndPopback(userPointLights, idx);
			}
		}
	}

	const SA::sp<SA::PointLight_Deferred> RenderSystem::createPointLight()
	{
		sp<PointLight_Deferred> newPointLight = new_sp<PointLight_Deferred>(PointLight_Deferred::PrivateConstructionKey{});

		userPointLights.push_back(newPointLight);

		return newPointLight;
	}

	bool RenderSystem::isUsingHDR()
	{
		if (deferredRenderer) { return true; }
		else if (forwardRenderer) { return forwardRenderer->isUsingHDR(); }
		else { return false; }
	}

}

