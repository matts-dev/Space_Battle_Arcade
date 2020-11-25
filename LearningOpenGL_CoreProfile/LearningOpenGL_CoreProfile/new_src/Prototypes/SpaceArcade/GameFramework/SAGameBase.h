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
	class RNGSystem;
	class AutomatedTestSystem;
	class DebugRenderSystem;
	class RenderSystem;
	class CurveSystem;
	class AudioSystem;

	class Window;
	
	class CheatSystemBase;

	struct TickGroups;
	class TickGroupManager;

	//////////////////////////////////////////////////////////////////////////////////////
	struct EngineConstants
	{
		int8_t RENDER_DELAY_FRAMES = 0;
		uint32_t MAX_DIR_LIGHTS = 4;
	};
	//////////////////////////////////////////////////////////////////////////////////////
	struct GamebaseIdentityKey : public RemoveCopies, public RemoveMoves
	{
		friend class GameBase; //only the game base can construct this.
	private:
		GamebaseIdentityKey() {}
	};
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
		~GameBase();

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
		void startShutdown();
		static bool isEngineShutdown();
		bool isExiting() { return bExitGame; };
		MultiDelegate<> onShutdownInitiated;
	protected:
		/** Child game classes should set up pre-gameloop state here.
			#return value Provide an initial primary window on startup.	*/
		virtual sp<Window> makeInitialWindow() = 0;
		virtual void startUp() = 0;
		virtual void onShutDown() = 0;
	private: //starting systems
		bool bStarted = false;
		bool bExitGame = false;


	//////////////////////////////////////////////////////////////////////////////////////
	//  GAME LOOP
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		MultiDelegate<> onGameloopBeginning;
		MultiDelegate<> onShutdownGameloopTicksOver;
		MultiDelegate<float /*deltaSec*/> onPreGameloopTick;
		MultiDelegate<float /*deltaSec*/> onPostGameloopTick;
		MultiDelegate<float /*deltaSec*/> onRenderDispatch;
		MultiDelegate<float /*deltaSec*/> onRenderDispatchEnded;
		MultiDelegate<uint64_t /*endingFrameNumber*/> onFrameOver;

	private: 
		void tickGameloop_GameBase();
	protected:
		virtual void tickGameLoop(float deltaTimeSecs) = 0;
		virtual void cacheRenderDataForCurrentFrame(struct RenderData& frameRenderData) = 0;
		virtual void renderLoop_begin(float deltaTimeSecs) = 0;
		virtual void renderLoop_end(float deltaTimeSecs) = 0;

	//////////////////////////////////////////////////////////////////////////////////////
	//  SYSTEMS 
	//		Each system has an explicit getter to for a reason. The core of this engine 
	//		and the systems at play can be understood by simply reading this header. 
	//		if making a templated getter (like with game components) will make it harder
	//		to get a bigger picture of the interplay of systems, in my opinion.
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		//System getters (to prevent circular dependencies, be sure to use forward declared references)
		inline WindowSystem& getWindowSystem() noexcept { return *windowSystem; }
		inline AssetSystem& getAssetSystem() noexcept { return *assetSystem; }
		inline LevelSystem& getLevelSystem() noexcept { return *levelSystem; }
		inline PlayerSystem& getPlayerSystem() noexcept { return *playerSystem; }
		inline ParticleSystem& getParticleSystem() noexcept { return *particleSystem;}
		inline RNGSystem& getRNGSystem() noexcept { return *systemRNG; }
		inline DebugRenderSystem& getDebugRenderSystem() noexcept { return *debugRenderSystem; }
		inline RenderSystem& getRenderSystem() noexcept { return *renderSystem; }
		inline AutomatedTestSystem& getAutomatedTestSystem() noexcept { return *automatedTestSystem;  };
		inline CheatSystemBase& getCheatSystem() { return *cheatSystem; }
		inline CurveSystem& getCurveSystem() { return *curveSystem; }
		inline AudioSystem& getAudioSystem() { return *audioSystem; }
	private:
		void createEngineSystems();
		/**polymorphic systems require virtual override to define class. If nullptr detected these systems should create a default instance.*/
		virtual sp<CheatSystemBase> createCheatSystemSubclass(){ return nullptr;}
		virtual sp<CurveSystem> createCurveSystemSubclass();
		virtual sp<AudioSystem> createAudioSystemSubclass();
	protected:
		virtual void onRegisterCustomSystem() {};
		void RegisterCustomSystem(const sp<SystemBase>& system);
	public:
		void subscribePostRender(const sp<SystemBase>& system); // this isn't as encapsulated as I'd like, but will not likely be an issue 
	private:
		bool bCustomSystemRegistrationAllowedTimeWindow = false;

	private: //systems
		sp<WindowSystem> windowSystem;
		sp<AssetSystem> assetSystem;
		sp<LevelSystem> levelSystem;
		sp<PlayerSystem> playerSystem;
		sp<ParticleSystem> particleSystem;
		sp<RNGSystem> systemRNG;
		sp<AutomatedTestSystem> automatedTestSystem;
		sp<DebugRenderSystem> debugRenderSystem;
		sp<RenderSystem> renderSystem;
		sp<CheatSystemBase> cheatSystem;
		sp<CurveSystem> curveSystem;
		sp<AudioSystem> audioSystem;

		std::set< sp<SystemBase> > systems;
		std::set< sp<SystemBase> > postRenderNotifys;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Constants
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	public:
		static inline const EngineConstants& getConstants() { return GameBase::get().configuredConstants; }
		/** Game subclasses can only configure game constants at this point early in initialization
			@Warning No systems will be initialized or available when this virtual is called.
		*/
		virtual void onInitEngineConstants(EngineConstants& config) {};
	private:
		EngineConstants configuredConstants;

	//////////////////////////////////////////////////////////////////////////////////////
	// frame id
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		uint64_t getFrameNumber() const noexcept { return frameNumber; }
	private:
		uint64_t frameNumber = 0;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Identity Key
	//		Systems that want to restrict function calls to GameBase without friending can require this object as a key
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	protected: //grant subclasses access to this key. This prevents anyone from passing the GameBase object as a key.
		GamebaseIdentityKey identityKey;

	//////////////////////////////////////////////////////////////////////////////////////
	//  Time
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		TimeSystem& getTimeSystem() { return timeSystem; }
		TimeManager& getSystemTimeManager(){ return *systemTimeManager; }
		TickGroups& tickGroups() { return *tickGroupData; }
		TickGroupManager& getTickGroupManager() { return *tickGroupManager; }
	private:
		void registerTickGroups();
		virtual sp<TickGroups> onRegisterTickGroups();
	private: //time management 
		/** Time management needs to be separate from systems since their tick relies on its results. */
		TimeSystem timeSystem;
		sp<TimeManager> systemTimeManager;
		sp<TickGroups> tickGroupData = nullptr;
		sp<TickGroupManager> tickGroupManager = nullptr;
		bool bTickGoupsInitialized = false;
	};

}