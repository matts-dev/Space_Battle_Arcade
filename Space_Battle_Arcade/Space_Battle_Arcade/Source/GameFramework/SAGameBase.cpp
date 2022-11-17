#include <iostream>

#include "SAGameBase.h"

#include "SASystemBase.h"
#include "SAWindowSystem.h"
#include "SAAssetSystem.h"
#include "SALevelSystem.h"
#include "SAPlayerSystem.h"
#include "SAParticleSystem.h"
#include "SAAutomatedTestSystem.h"

#include "../Rendering/SAWindow.h"
#include "SALog.h"
#include "SARandomNumberGenerationSystem.h"
#include "SADebugRenderSystem.h"
#include "SARenderSystem.h"
#include "CheatSystemBase.h"
#include "CurveSystem.h"
#include "TimeManagement/TickGroupManager.h"
#include "../Tools/PlatformUtils.h"
#include "SAAudioSystem.h"
#include <thread>
#include <chrono>
#include "../../../../Libraries/nlohmann/json.hpp"

namespace SA
{
	//globally available check for systems that may require engine services but will not in the event the engine has been destroyed.
	static bool bIsEngineShutdown = false;

	GameBase::GameBase()
	{
		bIsEngineShutdown = false;

		//allows subclasses to have local-static singleton getters
		if(!RegisteredSingleton)
		{
			RegisteredSingleton = this;
		}
		else
		{
			throw std::runtime_error("Only a single instance of the game can be created.");
		}

		//tick group manager comes before time system so that time system can set up tick groups
		tickGroupManager = new_sp<TickGroupManager>();
	}

	GameBase::~GameBase()
	{
		bIsEngineShutdown = true;
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
		//DEV-NOTE: this method should be kept simple, as it provides a high level overview of the engine.
		if (!bStarted)
		{
			onInitEngineConstants(configuredConstants);	//this should happen before the subclass game has started. this means systems can read it.
			registerTickGroups();						//tick groups created very early, these are effectively static and not intended to be initialized with dnyamic logic from systems. Thus these are created before systems.
			createEngineSystems();
			//systems are initialized after all systems have been created; this way cross-system interaction can be achieved during initailization (ie subscribing to events, etc.)
			for (const sp<SystemBase>& system : systems) { system->initSystem(); }

 			windowSystem->makeWindowPrimary(makeInitialWindow());
			startUp();
			bStarted = true;

			//game loop processes
			onGameloopBeginning.broadcast();
			while (!bExitGame) 
			{
				framerateSleep();
				tickGameloop_GameBase();
			}

			//begin shutdown process
			onShutDown();

			//shutdown systems after game client has been shutdown
			for (const sp<SystemBase>& system : systems){system->shutdown();}

			//tick a few more times for any frame deferred processes
			for (size_t shutdownTick = 0; shutdownTick < 3; ++shutdownTick){ tickGameloop_GameBase(); }

			onShutdownGameloopTicksOver.broadcast();
		}
	}

	bool GameBase::isEngineShutdown()
	{
		return bIsEngineShutdown;
	}

	void GameBase::startShutdown()
	{
		log("GameFramework", LogLevel::LOG, "Shutdown Initiated");
		onShutdownInitiated.broadcast();
		onShutDown();
		bExitGame = true;
	}

	void GameBase::tickGameloop_GameBase()
	{
		timeSystem.updateTime(TimeSystem::PrivateKey{});
		float deltaTimeSecs = systemTimeManager->getDeltaTimeSecs();

		GameEntity::cleanupPendingDestroy(GameEntity::CleanKey{});

		//the engine will tick a few times after shutdown to clean up deferred tasks.
		if (!bExitGame)
		{
			//#consider having system pass a reference to the system time manager, rather than a float; That way critical systems can ignore manipulation time effects or choose to use time affects. Passing raw time means systems will be forced to use time effects (such as dilation)
			for (const sp<SystemBase>& system : systems) { system->tick(deltaTimeSecs);	}

			//NOTE: there probably needs to be a priority based pre/post loop; but not needed yet so it is not implemented (priorities should probably be defined in a single file via template specliazations)
			onPreGameloopTick.broadcast(deltaTimeSecs);
			tickGameLoop(deltaTimeSecs);
			onPostGameloopTick.broadcast(deltaTimeSecs);

			cacheRenderDataForCurrentFrame(*renderSystem->getFrameRenderData_Write(frameNumber, identityKey));
			renderLoop_begin(deltaTimeSecs);
			onRenderDispatch.broadcast(deltaTimeSecs); //perhaps this needs to be a sorted structure with prioritizes; but that may get hard to maintain. Needs to be a systematic way for UI to come after other rendering.
			renderLoop_end(deltaTimeSecs);
			onRenderDispatchEnded.broadcast(deltaTimeSecs); 

			//perhaps this should be a subscription service since few systems care about post render //TODO this sytem should probably be removed and instead just subscribe to delegate
			for (const sp<SystemBase>& system : postRenderNotifys) { system->handlePostRender();}
		}

		//broadcast current frame and increment the frame number.
		onFrameOver.broadcast(frameNumber++);
	}

	void GameBase::createEngineSystems()
	{
		// !!! REFACTOR WARNING !!  do not place this within the ctor; polymorphic systems are designed to be instantiated via virtual functions; virutal functions shouldn't be called within a ctor!
		//this is provided outside of ctor so that virtual functions may be called to define systems that are polymorphic based on the game

		//initialize time management systems; this is done after tick groups are created so that timeManager can set up tick groups at construction
		systemTimeManager = timeSystem.createManager();
		timeSystem.markManagerCritical(TimeSystem::PrivateKey{}, systemTimeManager);

		//create and register systems
		windowSystem = new_sp<WindowSystem>();
		systems.insert(windowSystem);

		assetSystem = new_sp<AssetSystem>();
		systems.insert(assetSystem);

		levelSystem = new_sp<LevelSystem>();
		systems.insert(levelSystem);

		playerSystem = new_sp<PlayerSystem>();
		systems.insert(playerSystem);

		particleSystem = new_sp<ParticleSystem>();
		systems.insert(particleSystem);

		systemRNG = new_sp<RNGSystem>();
		systems.insert(systemRNG);

		debugRenderSystem = new_sp<DebugRenderSystem>();
		systems.insert(debugRenderSystem);

		renderSystem = new_sp<RenderSystem>();
		systems.insert(renderSystem);

		automatedTestSystem = new_sp<AutomatedTestSystem>();
		systems.insert(automatedTestSystem);

		cheatSystem = createCheatSystemSubclass();
		cheatSystem = cheatSystem ? cheatSystem : new_sp<CheatSystemBase>();
		systems.insert(cheatSystem);

		curveSystem = createCurveSystemSubclass();
		curveSystem = curveSystem ? curveSystem : new_sp<CurveSystem>();
		systems.insert(curveSystem);

		audioSystem = createAudioSystemSubclass();
		audioSystem = audioSystem ? audioSystem : new_sp<AudioSystem>();
		systems.insert(audioSystem);

		//initialize custom subclass systems; 
		//ctor warning: this is not done in gamebase ctor because it systems may call gamebase virtuals
		bCustomSystemRegistrationAllowedTimeWindow = true;
		onRegisterCustomSystem();
		bCustomSystemRegistrationAllowedTimeWindow = false;
	}

	sp<SA::CurveSystem> GameBase::createCurveSystemSubclass()
	{
		return new_sp<CurveSystem>();
	}

	sp<SA::AudioSystem> GameBase::createAudioSystemSubclass()
	{
		return new_sp<AudioSystem>();
	}

	void GameBase::subscribePostRender(const sp<SystemBase>& system)
	{
		postRenderNotifys.insert(system);
	}

	void GameBase::framerateSleep()
	{
		if (!bEnableFramerateLimit){return;}

		double currentTimeBeforeSleep = glfwGetTime();
		double deltaSec = currentTimeBeforeSleep - lastFrameTime;

		double frameTargetDeltaSec = 1.0 / double(targetFramesPerSecond); 
		
		bool bShouldSleep = frameTargetDeltaSec > deltaSec;
		double waitTimeSec = frameTargetDeltaSec - deltaSec;
		long long waitTimeMiliSec = static_cast<long long>(waitTimeSec * 1000.f);
		if (bShouldSleep)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMiliSec));
		}

		lastFrameTime = glfwGetTime();
		if (bShouldSleep && lastFrameTime == currentTimeBeforeSleep)
		{
			STOP_DEBUGGER_HERE(); // helps catch if on other drivers glfw isn't updating time after the sleep, if this is happening I may need to rething this framerate cap method
		}
	}

	void GameBase::registerTickGroups()
	{
		tickGroupManager->start_TickGroupRegistration(TickGroupManager::GameBaseKey{});

		//give users change to create special subclass with extra tick groups.
		tickGroupData = onRegisterTickGroups();
		if (!tickGroupData) 
		{
			STOP_DEBUGGER_HERE(); //subclass did not provide a subclass, providing default one. This is not expected behavior.
			tickGroupData = new_sp<TickGroups>();
		}

		tickGroupManager->stop_TickGroupRegistration(TickGroupManager::GameBaseKey{});
	}

	SA::sp<SA::TickGroups> GameBase::onRegisterTickGroups()
	{
		return new_sp<TickGroups>();
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



}


