#include "SAGameBase.h"
#include "SASubsystemBase.h"
#include "SAWindowSubsystem.h"
#include "..\Rendering\SAWindow.h"

namespace SA
{

	GameBase::GameBase()
	{
		//create and register subsystems
		windowSS = new_sp<WindowSubsystem>();
		subsystems.insert(windowSS);

	}

	void GameBase::start()
	{
		if (!bStarted)
		{
			bStarted = true;

			sp<Window> window = startUp();
			windowSS->makeWindowPrimary(window);

			//game loop processes
			while (!bExitGame)
			{
				TickGameloop();
			}

			//begin shutdown process
			
		}
	}

	void GameBase::TickGameloop()
	{
		updateTime();

		for (sp<SubsystemBase> subsystem : subsystems)
		{
			subsystem->tick(deltaTimeSecs);
		}
		TickGameLoopDerived(deltaTimeSecs);
	}

	void GameBase::updateTime()
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		deltaTimeSecs = timeDialoationFactor * rawDeltaTimeSecs;
		lastFrameTime = currentTime;
	}

}


