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
#include "../../GameFramework/TimeManagement/TickGroupManager.h"
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../Tools/Algorithms/Algorithms.h"
#include "../../Tools/SAUtilities.h"
#include "../../Tools/Geometry/Plane.h"
#include "../../Tools/Geometry/GeometryMath.h"

namespace SA
{
	////////////////////////////////////////////////////////
	// UI SYSTEM
	////////////////////////////////////////////////////////

	struct UISystem_Game::Internal
	{
		//ray casting
		std::vector<sp<const SH::HashCell<IMouseInteractable>>> rayHitCells;
		const float MAX_RAY_DIST = 1000.f;
		bool bClickNextRaycast = false; //when a click is detected, it is deferred until raycast happens.
		bool bMouseHeld = false;

		//used for tracking a previous click for dragging and release events.
		wp<SH::GridNode<IMouseInteractable>> previousClick;
		Plane dragPlane;

		//debug
		glm::vec3 rayStart;
		glm::vec3 rayEnd;
	};

	static DCFont::BatchData batchData;

	UISystem_Game::UISystem_Game()
	{
		impl = new_up<Internal>();
		impl->rayHitCells.reserve(100);
	}

	UISystem_Game::~UISystem_Game()
	{
		//define so deletion of impl is done in a translation unit that can see the definition
	}

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
		//start logic guard
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

	void UISystem_Game::postCameraTick(float dt_sec)
	{
		constexpr bool bDrawPickRays = false;//debug
		
		//since camera ticking is over, it isn't going to move. Shoot a ray and see if we hit anything in hitest grid... if we're in cursor mode.
		const sp<Window>& window = GameBase::get().getWindowSystem().getPrimaryWindow();
		const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0);
		if (window && player)
		{
			const sp<CameraBase>& camera = player->getCamera();
			if (camera && camera->isInCursorMode())
			{
				//#TODO taking from  HitboxPicker::handleRightClick, some of this may can be generalized into a function? just different enough that maybe not
				int screenWidth = 0, screenHeight = 0;
				double cursorPosX = 0.0, cursorPosY = 0.0;
				glfwGetWindowSize(window->get(), &screenWidth, &screenHeight);
				glfwGetCursorPos(window->get(), &cursorPosX, &cursorPosY);

				glm::vec2 mousePos_TopLeft(static_cast<float>(cursorPosX), static_cast<float>(cursorPosY));
				glm::vec2 screenResolution(static_cast<float>(screenWidth), static_cast<float>(screenHeight));

				glm::vec3 rayNudge = camera->getRight() * 0.001f; //nude ray to side so visible when debugging

				glm::vec3 camPos = camera->getPosition() + rayNudge;
				Ray clickRay = ObjectPicker::generateRay(
					glm::ivec2(screenWidth, screenHeight), mousePos_TopLeft,
					camPos, camera->getUp(), camera->getRight(), camera->getFront(), camera->getFOV(),
					window->getAspect());

				impl->rayHitCells.clear();
				spatialHashGrid.lookupCellsForLine(clickRay.start, clickRay.start + (clickRay.dir * impl->MAX_RAY_DIST), impl->rayHitCells);

				float closestDistance2SoFar = std::numeric_limits<float>::infinity();
				IMouseInteractable* closestPick = nullptr;
				sp<SH::GridNode<IMouseInteractable>> closestPickNode = nullptr;
				
				const bool bDragging = !impl->previousClick.expired();
				if (!bDragging)
				{
					//we're not dragging, see if we're hovering or clicking
					for (sp <const SH::HashCell<IMouseInteractable>>& hitCell : impl->rayHitCells)
					{
						for (sp<SH::GridNode<IMouseInteractable>> entityNode : hitCell->nodeBucket)
						{
							if (entityNode->element.isHitTestable())
							{
								glm::vec3 worldPos = entityNode->element.getWorldLocation();
								float distance2 = glm::length2(worldPos - camPos);
								if (distance2 < closestDistance2SoFar)
								{
									glm::mat4 inverseTransform = glm::inverse(entityNode->element.getModelMatrix());
									glm::vec4 transformedStart = inverseTransform * glm::vec4(clickRay.start, 1.f);
									glm::vec4 transformedDir = inverseTransform * glm::vec4(clickRay.dir, 0.f);

									const std::array<glm::vec4, 8>& localAABB = entityNode->element.getLocalAABB();
									glm::vec3 boxLow = Utils::findBoxLow(localAABB);
									glm::vec3 boxMax = Utils::findBoxMax(localAABB);
									if (Utils::rayHitTest_FastAABB(boxLow, boxMax, transformedStart, transformedDir))
									{
										closestPickNode = entityNode;
										closestPick = &entityNode->element;
										closestDistance2SoFar = distance2;
									}
								}
							}
						}
					}

					if (closestPick)
					{
						if (impl->bClickNextRaycast)
						{
							closestPick->onClickedThisTick();
							if (closestPick->supportsMouseDrag()) 
							{
								impl->previousClick = closestPickNode; /*cache for next tick drag updates*/
								impl->dragPlane.point = closestPick->getWorldLocation();
								impl->dragPlane.normal_v = -camera->getFront();
							}
						}
						else
						{
							closestPick->onHoveredhisTick();
						}
					}
				}
				else /*if dragging*/
				{
					sp<SH::GridNode<IMouseInteractable>> previousClickNode = impl->previousClick.lock();
					assert(previousClickNode); //should be impossible to get into this branch if we don't have a previous click
					if (previousClickNode)
					{
						//we're dragging
						if (impl->bMouseHeld)
						{
							if (std::optional<glm::vec3> dragPoint = rayPlaneIntersection(impl->dragPlane, clickRay.start, clickRay.dir))
							{
								previousClickNode->element.onMouseDraggedThisTick(*dragPoint);
							}
							else
							{
								log(__FUNCTION__, LogLevel::LOG_WARNING, "Did not find a drag point when dragging a UI widget, this is likely dud to the ray being parallel with the drag plane; ignoring event");
							}
						}
						else
						{
							previousClickNode->element.onMouseDraggedReleased();

							//clear out this clicked node so that we can go back to clicking and hovering
							impl->previousClick.reset();
						}
					}
				}


				if constexpr (bDrawPickRays)
				{
					impl->rayStart = clickRay.start;
					impl->rayEnd = clickRay.start + clickRay.dir * impl->MAX_RAY_DIST;
				}

				impl->rayHitCells.clear();
			}
		}

		if constexpr (bDrawPickRays)
		{
			static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
			debug.renderLine(impl->rayStart, impl->rayEnd, glm::vec3(1, 1, 1));
		}

		//always clear click, regardless if anything was hit; we don't want to click when dragging mouse over
		impl->bClickNextRaycast = false;
	}

	void UISystem_Game::initSystem()
	{
		DigitalClockFont::Data initBatcher;
		initBatcher.shader = getDefaultGlyphShader_instanceBased();
		defaultTextBatcher = new_sp<DigitalClockFont>(initBatcher);

		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &UISystem_Game::handlePreLevelChange);

		//this shouldn't happen as levels should be created after all systems are initialized, but adding this to prevent code fragility.
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			handlePreLevelChange(nullptr, currentLevel);
		}

		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		windowSystem.onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &UISystem_Game::handlePrimaryWindowChanged);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{

			handlePrimaryWindowChanged(nullptr, primaryWindow);
		}

	}

	void UISystem_Game::handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (previousLevel)
		{
			previousLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).removeWeak(sp_this(), &UISystem_Game::postCameraTick);
		}

		if (newCurrentLevel) 
		{
			newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).addWeakObj(sp_this(), &UISystem_Game::postCameraTick);
		}
	}

	void UISystem_Game::handlePrimaryWindowChanged(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		if (old_window)
		{
			new_window->onMouseButtonInput.removeWeak(sp_this(), &UISystem_Game::handleMouseButtonPressed);
		}

		if (new_window)
		{
			//alternatively we can bind to player input, but that seems wrong for a system like this. For now going straight to window.
			new_window->onMouseButtonInput.addWeakObj(sp_this(), &UISystem_Game::handleMouseButtonPressed);
		}

	}

	void UISystem_Game::handleMouseButtonPressed(int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (action == GLFW_PRESS)
			{
				impl->bClickNextRaycast = true;
				impl->bMouseHeld = true;
			}
			else
			{
				impl->bClickNextRaycast = false; //this is probably redundant as the click even will clear it; but perhaps this is the best way to clear it.
				impl->bMouseHeld = false;
			}
		}

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

	glm::vec3 GameUIRenderData::camRight()
	{
		if (!_camRight)
		{
			if (CameraBase* cam = camera())
			{
				_camRight = glm::vec3(cam->getRight());
			}

			if (!_camRight)
			{
				_camRight = glm::vec3{ 0,1,0 };
			}
		}

		return *_camRight;
	}

	glm::quat GameUIRenderData::camQuat()
	{
		if (!_camRot)
		{
			//dyn cast :( but cameras were not built to use transforms. Next time they will be!
			//if (QuaternionCamera* quatCam = dynamic_cast<QuaternionCamera*>(camera())) //adding getQuat to interface and testing
			if(CameraBase* quatCam = camera())
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
				_hudData3D->cameraNearPlane = gameCam->getNear();

				_hudData3D->textScale = 0.1f * (_hudData3D->frontOffsetDist / 10.f); //0.1 works good at distance 10.f; scale recommend text scale based on relation to 10
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

			//early engine boot up may call this before true aspect is set up. //#nextengine perhaps have functions that handle these which are set up before anything can access
			if (Utils::anyValueNAN(*_aspect))
			{
				_aspect = 1.f;
			}
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

