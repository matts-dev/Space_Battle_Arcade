#include <iostream>

#include "SAGameBase.h"

#include "SASystemBase.h"
#include "SAWindowSystem.h"
#include "SAAssetSystem.h"
#include "SALevelSystem.h"

#include "..\Rendering\SAWindow.h"
#include "SALog.h"
#include "SAPlayerSystem.h"

namespace SA
{

	GameBase::GameBase()
	{
		//allows subclasses to have local-static singleton getters
		if(!RegisteredSingleton)
		{
			RegisteredSingleton = this;
		}
		else
		{
			throw std::runtime_error("Only a single instance of the game can be created.");
		}
		
		//create and register systems
		windowSys = new_sp<WindowSystem>();
		systems.insert(windowSys);

		assetSys = new_sp<AssetSystem>();
		systems.insert(assetSys);

		levelSys = new_sp<LevelSystem>();
		systems.insert(levelSys);

		playerSys = new_sp<PlayerSystem>();
		systems.insert(playerSys);
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
		//WARNING: any local objects (eg smart pointers) in this function will have lifetime of game!
		if (!bStarted)
		{
			bStarted = true;

			//initialize systems; this is not done in gamebase ctor because it systemse may call gamebase virtuals
			bCustomSystemRegistrationAllowedTimeWindow = true;
			onRegisterCustomSystem();
			bCustomSystemRegistrationAllowedTimeWindow = false;
			for (const sp<SystemBase>& system : systems)
			{
				system->initSystem();
			}

			{ //prevent permanent window reference via scoped destruction
				sp<Window> window = startUp();
				windowSys->makeWindowPrimary(window);
			}

			//game loop processes
			while (!bExitGame)
			{
				TickGameloop_GameBase();
			}

			//begin shutdown process
			shutDown();

			//shutdown systems after game client has been shutdown
			for (const sp<SystemBase>& system : systems)
			{
				system->shutdown();
			}
		}
	}

	void GameBase::startShutdown()
	{
		log("Game : Shutdown Initiated", "GameFramework", LogLevel::LOG);
		bExitGame = true;
	}

	void GameBase::TickGameloop_GameBase()
	{
		updateTime();

		for (const sp<SystemBase>& system : systems) { system->tick(deltaTimeSecs);	}

		//NOTE: there probably needs to be a priority based pre/post loop; but not needed yet so it is not implemented (priorities should probably be defined in a single file via template specliazations)
		PreGameloopTick.broadcast(deltaTimeSecs);
		tickGameLoop(deltaTimeSecs);
		PostGameloopTick.broadcast(deltaTimeSecs);

		renderLoop(deltaTimeSecs);

		//perhaps this should be a subscription service since few systems care about post render
		for (const sp<SystemBase>& system : postRenderNotifys) { system->handlePostRender();}
	}

	void GameBase::SubscribePostRender(const sp<SystemBase>& system)
	{
		postRenderNotifys.insert(system);
	}

	void GameBase::RegisterCustomSystem(const sp<SystemBase>& system)
	{
		if (bCustomSystemRegistrationAllowedTimeWindow)
		{
			systems.insert(system);
		}
		else
		{
			std::cerr << "FATAL: attempting to register a custom system outside of start up window; use appropraite virtual function to register these systems" << std::endl;
		}
	}

	void GameBase::updateTime()
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		rawDeltaTimeSecs = rawDeltaTimeSecs > MAX_DELTA_TIME_SECS ? MAX_DELTA_TIME_SECS : rawDeltaTimeSecs;
		deltaTimeSecs = rawDeltaTimeSecs;
		lastFrameTime = currentTime;
	}

}


