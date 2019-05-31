#include <iostream>

#include "SAGameBase.h"

#include "SASubsystemBase.h"
#include "SAWindowSubsystem.h"
#include "SATextureSubsystem.h"
#include "SALevelSubsystem.h"

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

		textureSS = new_sp<TextureSubsystem>();
		subsystems.insert(textureSS);

		levelSS = new_sp<LevelSubsystem>();
		subsystems.insert(levelSS);
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
			bCustomSubsystemRegistrationAllowedTimeWindow = true;
			onRegisterCustomSubsystem();
			bCustomSubsystemRegistrationAllowedTimeWindow = false;
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
				TickGameloop_GameBase();
			}

			//begin shutdown process
			shutDown();

			//shutdown subsystems after game client has been shutdown
			for (const sp<SubsystemBase>& subsystem : subsystems)
			{
				subsystem->shutdown();
			}
		}
	}

	void GameBase::TickGameloop_GameBase()
	{
		updateTime();

		for (const sp<SubsystemBase>& subsystem : subsystems) { subsystem->tick(deltaTimeSecs);	}

		//NOTE: there probably needs to be a priority based pre/post loop; but not needed yet so it is not implemented (priorities should probably be defined in a single file via template specliazations)
		PreGameloopTick.broadcast(deltaTimeSecs);
		tickGameLoop(deltaTimeSecs);
		PostGameloopTick.broadcast(deltaTimeSecs);

		renderLoop(deltaTimeSecs);

		//perhaps this should be a subscription service since few systems care about post render
		for (const sp<SubsystemBase>& subsystem : postRenderNotifys) { subsystem->handlePostRender();}
	}

	void GameBase::SubscribePostRender(const sp < SubsystemBase>& subsystem)
	{
		postRenderNotifys.insert(subsystem);
	}

	void GameBase::RegisterCustomSubsystem(const sp <SubsystemBase>& subsystem)
	{
		if (bCustomSubsystemRegistrationAllowedTimeWindow)
		{
			subsystems.insert(subsystem);
		}
		else
		{
			std::cerr << "FATAL: attempting to register a custom subsystem outside of start up window; use appropraite virtual function to register these systems" << std::endl;
		}
	}

	void GameBase::updateTime()
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		deltaTimeSecs = timeDialoationFactor * rawDeltaTimeSecs;
		lastFrameTime = currentTime;
	}

}


