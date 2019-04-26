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
	public:
		GameBase();

		//Subsystem getters
		WindowSubsystem& getWindowSubsystem() { return *windowSS; }

	//////////////////////////////////////////////////////////////////////////////////////
	///  START UP
	/////////////////////////////////////////////////////////////////////////////////////
	public: //starting system
		/** Starts up systems and begins game loop */
		void start();
	protected:
		/** 
			Child game classes should set up pre-gameloop state here.

			#return value Provide an initial primary window on startup.
		*/
		virtual sp<Window> startUp() = 0;
	private: //starting systems
		bool bStarted = false;
		bool bExitGame = false;

	//////////////////////////////////////////////////////////////////////////////////////
	///  GAME LOOP
	//////////////////////////////////////////////////////////////////////////////////////
	private: 
		void TickGameloop();
	protected:
		virtual void TickGameLoopDerived(float deltaTimeSecs) = 0;
		void startShutdown() { bExitGame = true; }

	//////////////////////////////////////////////////////////////////////////////////////
	///  SUBSYSTEMS 
	//////////////////////////////////////////////////////////////////////////////////////
	private: //subsystems
		sp<WindowSubsystem> windowSS;
		std::set< sp<SubsystemBase> > subsystems;

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