#include <GLFW/glfw3.h>
#include "SATimeManagementSystem.h"
#include "SALog.h"

namespace SA
{


	void TimeManager::update(PrivateKey key, TimeSystem& timeSystem)
	{
		framesToStep = framesToStep > 0 ? framesToStep - 1 : framesToStep;
		if (framesToStep_nextFrame > 0)
		{
			framesToStep = framesToStep_nextFrame;
			framesToStep_nextFrame = 0;
		}

		//prevents time dilation from happening mid frame
		timeDilationFactor = DilationFactor_nextFrame;

		dt_undilatedSecs = timeSystem.getDeltaTimeSecs();
		dt_dilatedSecs = dt_undilatedSecs * timeDilationFactor;

	}

	void TimeSystem::updateTime(PrivateKey key)
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		rawDeltaTimeSecs = rawDeltaTimeSecs > MAX_DELTA_TIME_SECS ? MAX_DELTA_TIME_SECS : rawDeltaTimeSecs;
		deltaTimeSecs = rawDeltaTimeSecs;
		lastFrameTime = currentTime;

		for (const sp<TimeManager>& manager : managers)
		{
			manager->update(TimeManager::PrivateKey{}, *this);
		}
	}

	void TimeSystem::markManagerCritical(PrivateKey, sp<TimeManager>& manager)
	{
		criticalManagers.insert(manager);
	}

	sp<TimeManager> TimeSystem::createManager()
	{
		sp<TimeManager> newManager = new_sp<TimeManager>();

		managers.insert(newManager);

		return newManager;
	}

	void TimeSystem::destroyManager(sp<TimeManager>& manager)
	{
		if (criticalManagers.find(manager) == criticalManagers.end())
		{
			managers.erase(manager);
			manager = nullptr;
		}
		else
		{
			log("TimeSystem", LogLevel::LOG_WARNING, "Attempting to destroy a system critical time manager");
		}
	}

}

