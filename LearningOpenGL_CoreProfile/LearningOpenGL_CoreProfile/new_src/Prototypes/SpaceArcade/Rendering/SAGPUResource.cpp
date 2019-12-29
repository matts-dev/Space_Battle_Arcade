#include "SAGPUResource.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAWindowSystem.h"

namespace SA
{
	void GPUResource::postConstruct()
	{
		WindowSystem& windowSystem = GameBase::get().getWindowSystem();

		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &GPUResource::handleWindowLosingGPUContext);
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &GPUResource::handleWindowAcquiredGPUContext);

		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleWindowAcquiredGPUContext(primaryWindow);
		}
	}

	void GPUResource::handleWindowLosingGPUContext(const sp<Window>& window)
	{
		if (bAcquiredGPUResource)
		{
			onReleaseGPUResources();
			bAcquiredGPUResource = false;
		}
	}

	void GPUResource::handleWindowAcquiredGPUContext(const sp<Window>& window)
	{
		if (!bAcquiredGPUResource)
		{
			onAcquireGPUResources();
			bAcquiredGPUResource = true;
		}
	}


}

