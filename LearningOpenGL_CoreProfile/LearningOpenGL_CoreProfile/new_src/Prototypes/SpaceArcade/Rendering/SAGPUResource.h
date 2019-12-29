#pragma once

#include "../GameFramework/SAGameEntity.h"

namespace SA
{
	class Window;

	class GPUResource : public GameEntity
	{
	public:
		inline bool hasAcquiredResources() const { return bAcquiredGPUResource; }
	protected:
		virtual void postConstruct() override;
	private:
		void handleWindowLosingGPUContext(const sp<Window>& window);
		void handleWindowAcquiredGPUContext(const sp<Window>& window);
	private: //subclass api provided
		virtual void onReleaseGPUResources() = 0;
		virtual void onAcquireGPUResources() = 0;
	private:
		bool bAcquiredGPUResource = false;
	};
}