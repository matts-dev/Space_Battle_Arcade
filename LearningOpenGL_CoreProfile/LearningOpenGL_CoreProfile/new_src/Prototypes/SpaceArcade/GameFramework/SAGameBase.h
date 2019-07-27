#pragma once
#include <set>

#include "SAGameEntity.h"
#include "../Tools/RemoveSpecialMemberFunctionUtils.h"
#include "../Tools/DataStructures/MultiDelegate.h"
#include "SATimeManagementSystem.h"

namespace SA
{
	//forward declarations
	class SystemBase;
	class WindowSystem;
	class AssetSystem;
	class LevelSystem;
	class PlayerSystem;
	class ParticleSystem;
	class AutomatedTestSystem;

	class Window;

	//////////////////////////////////////////////////////////////////////////////////////

	/** 
		The Game Base class; this is the root to the game systems and has a static getter. 
		Understanding this class should be an early step in understanding the systems of
		this engine. 
	*/
	class GameBase : public GameEntity, public RemoveCopies, public RemoveMoves
	{

	//////////////////////////////////////////////////////////////////////////////////////
	//  Construction
	/////////////////////////////////////////////////////////////////////////////////////
	public:
		GameBase();

	//////////////////////////////////////////////////////////////////////////////////////
	//  Base Class Singleton
	/////////////////////////////////////////////////////////////////////////////////////
	public:
		static GameBase& get();

	private:
		/** It is preferred to make a static getter in the implemented subclass, but this
		    exists to avoid having systems know about concrete class types*/
		static GameBase* RegisteredSingleton;

	//////////////////////////////////////////////////////////////////////////////////////
	//  START UP / SHUT DOWN
	/////////////////////////////////////////////////////////////////////////////////////
	public: //starting system
		/** Starts up systems and begins game loop */
		void start();
	protected:
		/** Child game classes should set up pre-gameloop state here.
			#return value Provide an initial primary window on startup.	*/
		virtual sp<Window> startUp() = 0;
		virtual void shutDown() = 0;
	private: //starting systems
		bool bStarted = false;
		bool bExitGame = false;

	//////////////////////////////////////////////////////////////////////////////////////
	//  GAME LOOP
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		MultiDelegate<> onGameloopBeginning;
		MultiDelegate<float /*deltaSec*/> PreGameloopTick;
		MultiDelegate<float /*deltaSec*/> PostGameloopTick;

		void startShutdown();
	private: 
		void TickGameloop_GameBase();
	protected:
		virtual void tickGameLoop(float deltaTimeSecs) = 0;
		virtual void renderLoop(float deltaTimeSecs) = 0;

	//////////////////////////////////////////////////////////////////////////////////////
	//  SYSTEMS 
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		//System getters (to prevent circular dependencies, be sure to use forward declared references)
		// REFACTOR: returning raw reference like this allows for assignment; returning const sp<>& may be better, but may imply they're null (be impossible); Perhaps add shared_ref feature, that's sref<>, and return const sref<system>
		inline WindowSystem& getWindowSystem() noexcept { return *windowSystem; }
		inline AssetSystem& getAssetSystem() noexcept { return *assetSystem; }
		inline LevelSystem& getLevelSystem() noexcept { return *levelSystem; }
		inline PlayerSystem& getPlayerSystem() noexcept { return *playerSystem; }
		inline ParticleSystem& getParticleSystem() noexcept { return *particleSystem;}
		
		inline AutomatedTestSystem& getAutomatedTestSystem() noexcept { return *automatedTestSystem;  };

		/** this isn't as encapsulated as I'd like, but will not likely be an issue */
		void subscribePostRender(const sp<SystemBase>& system);
	private:
		bool bCustomSystemRegistrationAllowedTimeWindow = false;
	protected:
		virtual void onRegisterCustomSystem() {};
		void RegisterCustomSystem(const sp<SystemBase>& system);

	private: //systems
		sp<WindowSystem> windowSystem;
		sp<AssetSystem> assetSystem;
		sp<LevelSystem> levelSystem;
		sp<PlayerSystem> playerSystem;
		sp<ParticleSystem> particleSystem;
		sp<AutomatedTestSystem> automatedTestSystem;

		std::set< sp<SystemBase> > systems;
		std::set< sp<SystemBase> > postRenderNotifys;

	//////////////////////////////////////////////////////////////////////////////////////
	//  Time
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		TimeSystem& getTimeSystem() { return timeSystem; }
		TimeManager& getSystemTimeManager(){ return *systemTimeManager; }

	private: //time management 
		/** Time management needs to be separate from systems since their tick relies on its results. */
		TimeSystem timeSystem;
		sp<TimeManager> systemTimeManager;

	};

}