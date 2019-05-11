#pragma once
#include <set>

#include "SAGameEntity.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"

namespace SA
{
	//forward declarations
	class SubsystemBase;
	class WindowSubsystem;
	class Window;

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
		    exists to avoid having subsystems know about concrete class types*/
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
	private: 
		void TickGameloop_GameBase();
	protected:
		virtual void tickGameLoop(float deltaTimeSecs) = 0;
		void startShutdown() { bExitGame = true; }

	//////////////////////////////////////////////////////////////////////////////////////
	//  SUBSYSTEMS 
	//////////////////////////////////////////////////////////////////////////////////////
	public:
		//Subsystem getters
		WindowSubsystem& getWindowSubsystem() { return *windowSS; }

		/** this isn't as encapsulated as I'd like, but will not likely be an issue */
		void SubscribePostRender(const sp <SubsystemBase>& subsystem);
	private:
		bool bCustomSubsystemRegistrationAllowedTimeWindow = false;
	protected:
		virtual void onRegisterCustomSubsystem() {};
		void RegisterCustomSubsystem(const sp <SubsystemBase>& subsystem);

	private: //subsystems
		sp<WindowSubsystem> windowSS;
		std::set< sp<SubsystemBase> > subsystems;
		std::set< sp<SubsystemBase> > postRenderNotifys;
	
	//////////////////////////////////////////////////////////////////////////////////////
	//  Time
	//////////////////////////////////////////////////////////////////////////////////////

	private: //time management 
		/** Time management needs to be separate from subsystems since their tick relies on its results. */
		void updateTime();
		float currentTime = 0; 
		float lastFrameTime = 0;
		float rawDeltaTimeSecs = 0;
		float timeDialoationFactor = 1.0f;
		float deltaTimeSecs = 0.f;
	};

}