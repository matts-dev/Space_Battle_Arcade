#include <limits>

#include "../../../../../../Libraries/imgui.1.69.gl/imgui.h"

#include "SAHitboxPicker.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SAPlayerSystem.h"
#include "../../../GameFramework/SAPlayerBase.h"
#include "../../../GameFramework/Input/SAInput.h"
#include "../../../GameFramework/SALevel.h"
#include "../../../GameFramework/SALevelSystem.h"
#include "../../../GameFramework/SAWindowSystem.h"
#include "../../../Rendering/SAWindow.h"
#include "../../../Rendering/Camera/SACameraBase.h"
#include "../../../GameFramework/SACollisionUtils.h"
#include "../../../GameFramework/SALog.h"
#include "../../../GameFramework/SADebugRenderSystem.h"
#include "../../SpaceArcade.h"
#include "../../SAUISystem.h"
#include "../../../Tools/Algorithms/Algorithms.h"
#include "../../SAShip.h"
#include "../../../GameFramework/SABehaviorTree.h"
#include "../../../GameFramework/Components/GameplayComponents.h"
#include "../../../GameFramework/Components/CollisionComponent.h"

namespace SA
{
	namespace
	{
		glm::vec3 findBoxLow(const std::array<glm::vec4, 8>& localAABB)
		{
			glm::vec3 min = glm::vec3(std::numeric_limits<float>::infinity());
			for (const glm::vec4& vertex : localAABB)
			{
				min.x = vertex.x < min.x ? vertex.x : min.x;
				min.y = vertex.y < min.y ? vertex.y : min.y;
				min.z = vertex.z < min.z ? vertex.z : min.z;
			}
			return min;
		}

		glm::vec3 findBoxMax(const std::array<glm::vec4, 8>& localAABB)
		{
			glm::vec3 max = glm::vec3( -std::numeric_limits<float>::infinity() );
			for (const glm::vec4& vertex : localAABB)
			{
				max.x = vertex.x > max.x ? vertex.x : max.x;
				max.y = vertex.y > max.y ? vertex.y : max.y;
				max.z = vertex.z > max.z ? vertex.z : max.z;
			}
			return max;
		}

	}

	void HitboxPicker::postConstruct()
	{
		//this is the main reason it not in the gameframework level, the UI system currently is a game-specific system. Perhaps should change that.
		SpaceArcade& game = SpaceArcade::get();
		LevelSystem& levelSystem = game.getLevelSystem();

		game.getPlayerSystem().getPlayer(0)->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_RIGHT).addWeakObj(sp_this(), &HitboxPicker::handleRightClick);
		game.getUISystem()->onUIFrameStarted.addWeakObj(sp_this(), &HitboxPicker::handleUIFrameStarted);
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
							//make sure ray actually hit worldobject, and not just the cell that the worldobject is in.

							//use the AABB
							const CollisionInfo* collisionInfo = entityNode->element.getGameComponent<CollisionComponent>()->getCollisionData(); //component should exist if we've found them in spatial hash

							glm::mat4 inverseTransform = glm::inverse(entityNode->element.getModelMatrix());
							glm::vec4 transformedStart = inverseTransform * glm::vec4(clickRay.start, 1.f);
							glm::vec4 transformedDir = inverseTransform * glm::vec4(clickRay.dir, 0.f);

							const std::array<glm::vec4, 8>& localAABB = collisionInfo->getLocalAABB();
							glm::vec3 boxLow = findBoxLow(localAABB);
							glm::vec3 boxMax = findBoxMax(localAABB);

							if (rayHitTest_FastAABB(boxLow, boxMax, transformedStart, transformedDir))
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
			ImGui::TextWrapped("Hello Hitbox Picker");
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

	bool HitboxPicker::rayHitTest_FastAABB(const glm::vec3& boxLow, const glm::vec3& boxMax, const glm::vec3 rayStart, const glm::vec3 rayDir)
	{
		//FAST RAY BOX INTERSECTION
		//intersection = s + t*d;		where s is the start and d is the direction
		//for an axis aligned box, we can look at each axis individually
		//
		//intersection_x = s_x + t_x * d_x
		//intersection_y = s_y + t_y * d_y
		//intersection_z = s_z + t_z * d_z
		//
		//for each of those, we can solve for t
		//eg: (intersection_x - s_x) / d_z = t_z
		//intersection_x can be easily found since we have an axis aligned box, and there are 2 yz-planes that represent x values a ray will have to pass through
		//
		//intuitively, a ray that DOES intersect will pass through 3 planes before entering the cube; and pass through 3 planes to exit the cube.
		//the last plane it intersects when entering the cube, is the t value for the box intersection.
		//		eg ray goes through X plane, Y plane, and then Z Plane, the intersection point is the t value associated with the Z plane
		//the first plane it intersects when it leaves the box is also its exit intersection t value
		//		eg ray goes leaves Y plane, X plane, Z plane, then the intersection of the Y plane is the intersection point
		//if the object doesn't collide, then it will exit a plane before all 3 entrance places are intersected
		//		eg ray Enters X Plane, Enters Y plane, Exits X PLane, Enters Z plane, Exits Y plane, Exits Z plane; 
		//		there is no collision because it exited the x plane before it penetrated the z plane
		//it seems that, if it is within the cube, the entrance planes will all have negative t values

		////calculate bounding box dimensions; may need to nudge intersection pnt so it doesn't pick up previous box
		float fX = boxLow.x;
		float cX = boxMax.x;
		float fY = boxLow.y;
		float cY = boxMax.y;
		float fZ = boxLow.z;
		float cZ = boxMax.z;

		//use algbra to calculate T when these planes are hit; similar to above example -- looking at single components
		// pnt = s + t * d;			t = (pnt - s)/d
		//these calculations may produce infinity
		float tMaxX = (fX - rayStart.x) / rayDir.x;
		float tMinX = (cX - rayStart.x) / rayDir.x;
		float tMaxY = (fY - rayStart.y) / rayDir.y;
		float tMinY = (cY - rayStart.y) / rayDir.y;
		float tMaxZ = (fZ - rayStart.z) / rayDir.z;
		float tMinZ = (cZ - rayStart.z) / rayDir.z;

		float x_enter_t = std::min(tMinX, tMaxX);
		float x_exit_t = std::max(tMinX, tMaxX);
		float y_enter_t = std::min(tMinY, tMaxY);
		float y_exit_t = std::max(tMinY, tMaxY);
		float z_enter_t = std::min(tMinZ, tMaxZ);
		float z_exit_t = std::max(tMinZ, tMaxZ);

		float enterTs[3] = { x_enter_t, y_enter_t, z_enter_t };
		std::size_t numElements = sizeof(enterTs) / sizeof(enterTs[0]);
		std::sort(enterTs, enterTs + numElements);

		float exitTs[3] = { x_exit_t, y_exit_t, z_exit_t };
		std::sort(exitTs, exitTs + numElements);

		//handle cases where infinity is within enterT
		for (int idx = numElements - 1; idx >= 0; --idx)
		{
			if (enterTs[idx] != std::numeric_limits<float>::infinity())
			{
				//move a real value to the place where infinity was sorted
				enterTs[2] = enterTs[idx];
				break;
			}
		}

		bool intersects = enterTs[2] <= exitTs[0];
		float collisionT = enterTs[2];

		//calculate new intersection point!
		//glm::vec3 intersectPoint = start + collisionT * rayDir;

		return intersects;
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

