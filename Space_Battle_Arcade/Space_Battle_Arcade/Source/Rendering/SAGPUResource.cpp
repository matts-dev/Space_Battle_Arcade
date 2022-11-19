#include "Rendering/SAGPUResource.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAWindowSystem.h"

namespace SA
{

	void GPUResource::cleanup()
	{
		if (hasAcquiredResources())
		{
			onReleaseGPUResources();
		}
	}

	void GPUResource::postConstruct()
	{
		GameBase& game = GameBase::get();
		WindowSystem& windowSystem = game.getWindowSystem();

		game.onShutdownGameloopTicksOver.addWeakObj(sp_this(), &GPUResource::handlePostEngineShutdownTicksOver);
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
			bAcquiredGPUResource = false; //this should come after release so that it can be polled during release
		}
	}

	void GPUResource::handleWindowAcquiredGPUContext(const sp<Window>& window)
	{
		if (!bAcquiredGPUResource)
		{
			onAcquireGPUResources();
			bAcquiredGPUResource = true;  //this should come after acquire so that it can be polled during acquire
		}
	}


	void GPUResource::handlePostEngineShutdownTicksOver()
	{
		//on shutdown, release the GPU resources before systems (ie window system) is destroyed.
		//But let the game finish its shutdown ticks, which is why this doesn't happen on shutdown initiated.
		//creating dummy window so we flow through same codepath as normal gpu resource release
		const sp<Window> dummyWindow = nullptr;
		handleWindowLosingGPUContext(dummyWindow);
	}

}

