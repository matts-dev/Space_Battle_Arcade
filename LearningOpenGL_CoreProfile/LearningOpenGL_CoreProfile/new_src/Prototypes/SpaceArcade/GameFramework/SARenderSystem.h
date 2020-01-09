#pragma once
#include "SASystemBase.h"
#include <vector>

namespace SA
{
	struct RenderData;
	class GameBase;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The encapsulated render system
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class RenderSystem final : public SystemBase
	{
	public:
		/** Subclasses of GameBase can write to a frames data, whereas everyone else can only read from the data */
		const RenderData* getFrameRenderData_Read(uint64_t frameNumber)														{ return getFrameRenderData(frameNumber); }
		RenderData*		  getFrameRenderData_Write(uint64_t frameNumber, const struct GamebaseIdentityKey& privateKey)		{ return getFrameRenderData(frameNumber); }
	private:
		/** Private to allow deciding whether to return this as constant data or mutable data.*/
		RenderData* getFrameRenderData(uint64_t frameNumber);
	private:
		virtual void initSystem() override;
	private:
		std::vector<sp<RenderData>> renderFrameCircularBuffer;
	};
}