#include "SARenderSystem.h"
#include "SAGameBase.h"
#include "../Rendering/RenderData.h"

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


}

