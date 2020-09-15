#pragma once

#include "../GameFramework/SAGameEntity.h"

namespace SA
{
	class Window;

	//#TODO create system that detects post construct was not called and alert user; this could be mitigated with smarter events

	class GPUResource : public GameEntity
	{
	public:
		inline bool hasAcquiredResources() const { return bAcquiredGPUResource; }
		//virtual ~GPUResource(); //thinking about adding this? cleaning up via virtual functions in dtor is probably not what you want
		void cleanup();
	protected:
		virtual void postConstruct() override;
	private:
		void handleWindowLosingGPUContext(const sp<Window>& window);
		void handleWindowAcquiredGPUContext(const sp<Window>& window);
		void handlePostEngineShutdownTicksOver();
	private: //subclass api provided
		virtual void onReleaseGPUResources() = 0;
		virtual void onAcquireGPUResources() = 0;
	private:
		bool bAcquiredGPUResource = false;
	};
}