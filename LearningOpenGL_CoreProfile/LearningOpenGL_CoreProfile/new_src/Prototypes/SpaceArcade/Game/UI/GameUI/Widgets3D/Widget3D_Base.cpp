#include "Widget3D_Base.h"
#include "../../../../GameFramework/SALevelSystem.h"
#include "../../../../GameFramework/SALevel.h"
#include "../../../../GameFramework/SATimeManagementSystem.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../../GameFramework/SAWindowSystem.h"

namespace SA
{
	float GameUIRenderData::dt_sec()
	{
		static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();
		if (currentLevel)
		{
			if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
			{
				_dt_sec = worldTimeManager->getDeltaTimeSecs();
			}
		}

		if (!_dt_sec.has_value()) //failure, return sensible value
		{
			_dt_sec = 0.f;
		}

		return *_dt_sec;
	}

	glm::ivec2 GameUIRenderData::framebuffer_Size()
	{
		if (!_framebuffer_Size.has_value())
		{
			calculateFramebufferMetrics();
		}
		if (!_framebuffer_Size.has_value())
		{
			_framebuffer_Size = glm::ivec2(0, 0);
		}
		return *_framebuffer_Size;
	}

	int GameUIRenderData::frameBuffer_MinDimension()
{
		if (!_frameBuffer_MinDimension.has_value())
		{
			calculateFramebufferMetrics();
		}
		if (!_frameBuffer_MinDimension.has_value())
		{
			_frameBuffer_MinDimension = 0;
		}
		return *_frameBuffer_MinDimension;
	}

	glm::mat4 GameUIRenderData::orthographicProjection_m()
	{
		if (!_orthographicProjection_m)
		{
			glm::ivec2 fb = framebuffer_Size();

			_orthographicProjection_m = glm::ortho(
				-(float)fb.x / 2.0f, (float)fb.x/ 2.0f,
				-(float)fb.y/ 2.0f, (float)fb.y/ 2.0f
			);
		}

		return *_orthographicProjection_m;
	}

	void GameUIRenderData::calculateFramebufferMetrics()
	{
		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			const std::pair<int, int> framebufferSizePair = primaryWindow->getFramebufferSize();
			_framebuffer_Size = glm::ivec2{ framebufferSizePair.first, framebufferSizePair.second};
			_frameBuffer_MinDimension = glm::min<int>(_framebuffer_Size->x, _framebuffer_Size->y);
		}
	}

}

