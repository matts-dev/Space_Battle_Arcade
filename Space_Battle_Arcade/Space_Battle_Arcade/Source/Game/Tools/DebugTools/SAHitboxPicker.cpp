#include <limits>

#include "Libraries/imgui.1.69.gl/imgui.h"

#include "SAHitboxPicker.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAPlayerSystem.h"
#include "GameFramework/SAPlayerBase.h"
#include "GameFramework/Input/SAInput.h"
#include "GameFramework/SALevel.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAWindowSystem.h"
#include "Rendering/SAWindow.h"
#include "Rendering/Camera/SACameraBase.h"
#include "GameFramework/SACollisionUtils.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SADebugRenderSystem.h"
#include "Game/SpaceArcade.h"
#include "Game/GameSystems/SAUISystem_Editor.h"
#include "Tools/Algorithms/Algorithms.h"
#include "Game/SAShip.h"
#include "GameFramework/SABehaviorTree.h"
#include "GameFramework/Components/GameplayComponents.h"
#include "GameFramework/Components/CollisionComponent.h"
#include "Tools/SAUtilities.h"

namespace SA
{


	void HitboxPicker::postConstruct()
	{
		//this is the main reason it not in the gameframework level, the UI system currently is a game-specific system. Perhaps should change that.
		SpaceArcade& game = SpaceArcade::get();
		LevelSystem& levelSystem = game.getLevelSystem();

		game.getPlayerSystem().getPlayer(0)->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_RIGHT).addWeakObj(sp_this(), &HitboxPicker::handleRightClick);
		game.getPlayerSystem().getPlayer(0)->onControlTargetSet.addWeakObj(sp_this(), &HitboxPicker::handlePlayerControlTargetSet);
		game.getEditorUISystem()->onUIFrameStarted.addWeakObj(sp_this(), &HitboxPicker::handleUIFrameStarted);
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &HitboxPicker::handlePreLevelChange);
		if (sp<LevelBase> currentLevel = levelSystem.getCurrentLevel())
		{
			currentLevel->getWorldTimeManager()->registerTicker(sp_this());
		}

		
	}

	void HitboxPicker::handleRightClick(int state, int modifier_keys)
	{
		if (bPickingObject && state == GLFW_PRESS)
		{
			static GameBase& game = GameBase::get();
			static LevelSystem& levelSys = game.getLevelSystem();
			static PlayerSystem& playerSys = game.getPlayerSystem();
			static WindowSystem& windowSys = game.getWindowSystem();

			const sp<CameraBase>& camera = playerSys.getPlayer(0)->getCamera();
			const sp<LevelBase>& level = levelSys.getCurrentLevel();
			const sp<Window>& window = windowSys.getPrimaryWindow();
			if (camera && level && window)
			{
				//#future perhaps these functions should be off of the window object

				int screenWidth = 0, screenHeight = 0;
				double cursorPosX = 0.0, cursorPosY = 0.0;
				glfwGetWindowSize(window->get(), &screenWidth, &screenHeight);
				//glfwGetFramebufferSize(window->get(), &screenWidth, &screenHeight);
				glfwGetCursorPos(window->get(), &cursorPosX, &cursorPosY);

				glm::vec2 mousePos_TopLeft(static_cast<float>(cursorPosX), static_cast<float>(cursorPosY));
				glm::vec2 screenResolution(static_cast<float>(screenWidth), static_cast<float>(screenHeight));

				glm::vec3 rayNudge = camera->getRight() * 0.001f; //nude ray to side so visible when debugging

				glm::vec3 camPos = camera->getPosition() + rayNudge;
				Ray clickRay = ObjectPicker::generateRay(
					glm::ivec2(screenWidth, screenHeight), mousePos_TopLeft,
					camPos, camera->getUp(), camera->getRight(), camera->getFront(), camera->getFOV(),
					window->getAspect());

				SH::SpatialHashGrid<WorldEntity>& worldGrid = level->getWorldGrid();
				std::vector<sp<const SH::HashCell<WorldEntity>>> rayHitCells;
				worldGrid.lookupCellsForLine(clickRay.start, clickRay.start + (clickRay.dir * maxRayDistance), rayHitCells);

				float closestDistance2SoFar = std::numeric_limits<float>::infinity();
				WorldEntity* closestPick = nullptr;

				for (sp <const SH::HashCell<WorldEntity>>& hitCell : rayHitCells)
				{
					for (sp<SH::GridNode<WorldEntity>> entityNode : hitCell->nodeBucket)
					{
						glm::vec3 worldPos = entityNode->element.getWorldPosition();
						float distance2 = glm::length2(worldPos - camPos);
						if (distance2 < closestDistance2SoFar)
						{
							//make sure ray actually hit world object, and not just the cell that the worldobject is in.

							//use the AABB
							const CollisionData* collisionInfo = entityNode->element.getGameComponent<CollisionComponent>()->getCollisionData(); //component should exist if we've found them in spatial hash

							glm::mat4 inverseTransform = glm::inverse(entityNode->element.getModelMatrix());
							glm::vec4 transformedStart = inverseTransform * glm::vec4(clickRay.start, 1.f);
							glm::vec4 transformedDir = inverseTransform * glm::vec4(clickRay.dir, 0.f);

							const std::array<glm::vec4, 8>& localAABB = collisionInfo->getLocalAABB();
							glm::vec3 boxLow = Utils::findBoxLow(localAABB);
							glm::vec3 boxMax = Utils::findBoxMax(localAABB);

							if (Utils::rayHitTest_FastAABB(boxLow, boxMax, transformedStart, transformedDir))
							{
								closestPick = &entityNode->element;
								closestDistance2SoFar = distance2;
							}
						}
					}
				}
				if (closestPick)
				{
					wp<GameEntity> weakGM = closestPick->requestReference();
					if (!weakGM.expired())
					{
						sp<GameEntity> sGM = weakGM.lock();
						setPickTarget(std::dynamic_pointer_cast<WorldEntity>(sGM));

						if (bClearPickingAfterSuccessfulPick)
						{
							bPickingObject = pickedObject;
						}
						
					}
					else
					{
						log("HitboxPicker", LogLevel::LOG_WARNING, "picked object is denied reference handle request.");
					}

				}

				if (bDrawPickRays)
				{
					static DebugRenderSystem& debug = game.getDebugRenderSystem();
					rayStart = clickRay.start;
					rayEnd = clickRay.start + clickRay.dir * maxRayDistance;
				}
			}

			accumulatedTime = 0;
		}
	}


	void HitboxPicker::handlePlayerControlTargetSet(IControllable* oldTarget, IControllable* newTarget)
	{
		//player set control target, stop trying to follow as we don't need to set camera position anymore.
		//setting camera position will mess with respawn hud location if this moves camera.
		bCameraFollowTarget = false;
	}

	void HitboxPicker::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		bPickingObject = false;
		pickedObject = nullptr;

		if (currentLevel) { currentLevel->getWorldTimeManager()->removeTicker(sp_this()); }
		if (newLevel){ newLevel->getWorldTimeManager()->registerTicker(sp_this());}
	}

	void HitboxPicker::handleUIFrameStarted()
	{
		ImGui::SetNextWindowSize(ImVec2{ 200, 230 });
		ImGui::SetNextWindowPos(ImVec2{ 1000, 0 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		ImGui::Begin("Hitbox Picker Tool", nullptr, flags);
		{
			ImGui::SetWindowCollapsed(true, ImGuiCond_Once);
			ImGui::TextWrapped("%s", "Hello Hitbox Picker");
			ImGui::Checkbox("draw ray casts", &bDrawPickRays);
			ImGui::Checkbox("enable right click pick", &bPickingObject);
			ImGui::Checkbox("stop picking after find", &bClearPickingAfterSuccessfulPick);
			ImGui::Checkbox("camera follow target", &bCameraFollowTarget);
			ImGui::Checkbox("\tcamera locked behind target", &bCameraFollowTarget_behind);
			
			if (ImGui::Button("Clear Pick Reference")) { pickedObject = nullptr; }
			if (ImGui::Button("Debug Behavior Tree")) { debugBehaviorTree(); }
			ImGui::Text("Picked obj: %p", pickedObject ? pickedObject.get() : nullptr);


		}
		ImGui::End();
	}

	void HitboxPicker::debugBehaviorTree()
	{
		if (BrainComponent* brainComp = pickedObject->getGameComponent<BrainComponent>())
		{
			if (const BehaviorTree::Tree* tree = brainComp->getTree())
			{
				tree->makeTreeDebugTarget();
			}
		}
	}

	bool HitboxPicker::tick(float dt_sec)
	{
		accumulatedTime += dt_sec;

		if (bDrawPickRays && accumulatedTime < renderDuration)
		{
			static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
			debug.renderLine(rayStart, rayEnd, glm::vec3(0, 1, 0));
		}

		if (bCameraFollowTarget && pickedObject)
		{
			static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
			if (const sp<CameraBase>& camera = playerSystem.getPlayer(0)->getCamera())
			{
				glm::vec3 objPos = pickedObject->getWorldPosition();

				if (bCameraFollowTarget_behind)
				{
					glm::vec4 newFront = pickedObject->getTransform().getModelMatrix() * glm::vec4(1, 0, 0, 1);
					camera->lookAt_v(camera->getPosition() + glm::vec3(newFront));
				}
				glm::vec3 newCamPos = objPos + cameraDistance * -camera->getFront();
				camera->setPosition(newCamPos);
				camera->lookAt_v(objPos);
			}
		}

		//stop ticking after time is up
		return true;
	}

}

