#include "SAGameBase.h"
#include "SASubsystemBase.h"
#include "SAWindowSubsystem.h"
#include "..\Rendering\SAWindow.h"

namespace SA
{

	GameBase::GameBase()
	{
		if(!RegisteredSingleton)
		{
			RegisteredSingleton = this;
		}
		else
		{
			throw std::runtime_error("Only a single instance of the game can be created.");
		}

		//create and register subsystems
		windowSS = new_sp<WindowSubsystem>();
		subsystems.insert(windowSS);
	}

	GameBase* GameBase::RegisteredSingleton = nullptr;
	SA::GameBase& GameBase::get()
	{
		if (!RegisteredSingleton)
		{
			throw std::runtime_error("GAME BASE NOT CREATED");
		}
		return *RegisteredSingleton;
	}

	void GameBase::start()
	{
		//WARNING: any local objects (eg smart pointers) in this function will have lifttime of game!
		if (!bStarted)
		{
			bStarted = true;

			//initialize subsystems; this is not done in gamebase ctor because it subsystemse may call gamebase virtuals
			for (const sp<SubsystemBase>& subsystem : subsystems)
			{
				subsystem->initSystem();
			}

			{ //prevent permanent window reference via scoped destruction
				sp<Window> window = startUp();
				windowSS->makeWindowPrimary(window);
			}

			//game loop processes
			while (!bExitGame)
			{
				TickGameloop();
			}

			//begin shutdown process
			shutDown();

			//shutdown subsystems after game client has been shutdown
			//currently not needed, but will do it here when it is needed
		}
	}

	void GameBase::TickGameloop()
	{
		updateTime();

		for (const sp<SubsystemBase>& subsystem : subsystems)
		{
			subsystem->tick(deltaTimeSecs);
		}

		tickGameLoopDerived(deltaTimeSecs);

		//perhaps this should be a subscription service since few systems care about post render
		for (const sp<SubsystemBase>& subsystem : postRenderNotifys)
		{
			subsystem->handlePostRender();
		}
	}

	void GameBase::SubscribePostRender(const sp < SubsystemBase>& subsystem)
	{
		postRenderNotifys.insert(subsystem);
	}

	void GameBase::updateTime()
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		deltaTimeSecs = timeDialoationFactor * rawDeltaTimeSecs;
		lastFrameTime = currentTime;
	}

}


