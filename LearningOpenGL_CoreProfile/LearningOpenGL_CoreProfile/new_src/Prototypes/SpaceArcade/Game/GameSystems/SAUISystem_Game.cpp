#include "SAUISystem_Game.h"

#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SATimeManagementSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../UI/GameUI/text/DigitalClockFont.h"
#include "../../Rendering/RenderData.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Tools/PlatformUtils.h"

namespace SA
{
	////////////////////////////////////////////////////////
	// UI SYSTEM
	////////////////////////////////////////////////////////

	static DCFont::BatchData batchData;

	void UISystem_Game::batchToDefaultText(DigitalClockFont& text, GameUIRenderData& ui_rd)
	{
		assert(bRenderingGameUI); //if we are not rendering to UI, we should not call batch text as it MAY render if buffers are full; this must not happen during non-rendering code as it will cause corruptions
		
		if (!defaultTextBatcher->prepareBatchedInstance(text, batchData))
		{
			//buffers are full, commit batch to render, then batch
			if (const RenderData* rd = ui_rd.renderData())
			{
				defaultTextBatcher->renderBatched(*rd, batchData);
			}

			if (!defaultTextBatcher->prepareBatchedInstance(text, batchData))
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "impossible request for text buffer, or no render data available");
				STOP_DEBUGGER_HERE();
			}
		}
	}

	void UISystem_Game::runGameUIPass() const
	{
		//start logic gaurd
		bRenderingGameUI = true;

		GameUIRenderData uiRenderData;
		batchData = DCFont::BatchData{}; //clear batch data
		onUIGameRender.broadcast(uiRenderData);

		//commit any pending batch renders
		if (const RenderData* renderData = uiRenderData.renderData())
		{
			defaultTextBatcher->renderBatched(*renderData, batchData);
		}

		//cleanup logic guard
		bRenderingGameUI = true;
	}


	void UISystem_Game::initSystem()
	{
		DigitalClockFont::Data initBatcher;
		initBatcher.shader = getDefaultGlyphShader_instanceBased();
		defaultTextBatcher = new_sp<DigitalClockFont>(initBatcher);
	}

	////////////////////////////////////////////////////////
	// RENDER DATA
	////////////////////////////////////////////////////////

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

			if (!_dt_sec.has_value()) //failure, return sensible value
			{
				_dt_sec = 0.f;
			}
		}

		return *_dt_sec;
	}

	glm::ivec2 GameUIRenderData::framebuffer_Size()
	{
		if (!_framebuffer_Size.has_value())
		{
			calculateFramebufferMetrics();
			if (!_framebuffer_Size.has_value())
			{
				_framebuffer_Size = glm::ivec2(0, 0);
			}
		}
		return *_framebuffer_Size;
	}

	int GameUIRenderData::frameBuffer_MinDimension()
	{
		if (!_frameBuffer_MinDimension.has_value())
		{
			calculateFramebufferMetrics();

			if (!_frameBuffer_MinDimension.has_value())
			{
				_frameBuffer_MinDimension = 0;
			}
		}
		return *_frameBuffer_MinDimension;
	}

	glm::mat4 GameUIRenderData::orthographicProjection_m()
	{
		if (!_orthographicProjection_m)
		{
			glm::ivec2 fb = framebuffer_Size();

			_orthographicProjection_m = glm::ortho(
				-(float)fb.x / 2.0f, (float)fb.x / 2.0f,
				-(float)fb.y / 2.0f, (float)fb.y / 2.0f
			);
		}

		return *_orthographicProjection_m;
	}

	SA::CameraBase* GameUIRenderData::camera()
	{
		if (!_camera.has_value())
		{
			PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
			if (const sp<PlayerBase>& player = playerSystem.getPlayer(0))
			{
				_camera = player->getCamera();
			}

			if (!_camera)
			{
				_camera = nullptr;
			}
		}

		return (*_camera).get();
	}

	glm::vec3 GameUIRenderData::camUp()
	{
		if (!_camUp)
		{
			if (CameraBase* cam = camera())
			{
				_camUp = glm::vec3(cam->getUp());
			}

			if (!_camUp)
			{
				_camUp = glm::vec3{ 0,1,0 };
			}
		}

		return *_camUp;
	}

	glm::quat GameUIRenderData::camQuat()
	{
		if (!_camRot)
		{
			//dyn cast :( but cameras were not built to use transforms. Next time they will be!
			if (QuaternionCamera* quatCam = dynamic_cast<QuaternionCamera*>(camera()))
			{
				_camRot = quatCam->getQuat();
			}

			//if we didn't find it, cache a unit quaternion
			if (!_camRot)
			{
				_camRot = glm::quat(1, 0, 0, 0);
			}
		}

		return *_camRot;
	}

	const SA::RenderData* GameUIRenderData::renderData()
	{
		if (!_frameRenderData.has_value())
		{
			GameBase& game = GameBase::get();
			if (const RenderData* frameRenderData = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
			{
				_frameRenderData = frameRenderData;
			}

			//if failed to get frame render data return nullptr
			if (!_frameRenderData.has_value())
			{
				_frameRenderData = nullptr;
			}
		}

		return *_frameRenderData;
	}

	const SA::HUDData3D& GameUIRenderData::getHUDData3D()
	{
		if (!_hudData3D)
		{
			_hudData3D = HUDData3D{};

			if (SA::CameraBase* gameCam = camera())
			{
				_hudData3D->camPos = gameCam->getPosition();
				_hudData3D->camUp = gameCam->getUp();
				_hudData3D->camRight = gameCam->getRight();
				_hudData3D->camFront = gameCam->getFront();

				float FOVy_deg = gameCam->getFOV() / 2.f;
				float FOVy_rad = glm::radians(FOVy_deg);

				//drawing out triangle with fovy, tan(theta) = y / z;
				// y = tan(theta) * z
				_hudData3D->savezoneMax_y = glm::tan(FOVy_rad) * _hudData3D->frontOffsetDist;
				_hudData3D->savezoneMax_x = _hudData3D->savezoneMax_y * aspect();

			}

			if (!_hudData3D)
			{
				_hudData3D = HUDData3D{};
			}
		}

		return *_hudData3D;
	}

	float GameUIRenderData::aspect()
	{
		if (!_aspect)
		{
			glm::ivec2 fbSize = framebuffer_Size();
			_aspect = fbSize.x / float(fbSize.y);
		}

		return *_aspect;
	}

	void GameUIRenderData::calculateFramebufferMetrics()
	{
		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			const std::pair<int, int> framebufferSizePair = primaryWindow->getFramebufferSize();
			_framebuffer_Size = glm::ivec2{ framebufferSizePair.first, framebufferSizePair.second };
			_frameBuffer_MinDimension = glm::min<int>(_framebuffer_Size->x, _framebuffer_Size->y);
		}
	}


}

