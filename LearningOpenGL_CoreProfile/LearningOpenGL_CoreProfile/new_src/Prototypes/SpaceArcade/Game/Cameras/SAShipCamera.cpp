
#include "SAShipCamera.h"
#include "../SAShip.h"
#include "../SpaceArcade.h"
#include "../SAUISystem.h"
#include "../SAPlayer.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/color_utils.h"
#include "../../Tools/SAUtilities.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"

namespace SA
{
	using namespace glm;

	void ShipCamera::followShip(const sp<Ship>& ship)
	{
		if (myShip)
		{
			myShip->onTransformUpdated.removeWeak(sp_this(), &ShipCamera::handleShipTransformChanged);
		}

		myShip = ship;

		if (ship)
		{
			ship->onTransformUpdated.addWeakObj(sp_this(), &ShipCamera::handleShipTransformChanged);
			handleShipTransformChanged(ship->getTransform());
		}
	}

	void ShipCamera::tickKeyboardInput(float dt_sec)
	{
		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow();
		if (primaryWindow && myShip)
		{
			GLFWwindow* window = primaryWindow->get();

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				myShip->setNextFrameBoost(2.0f);
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				myShip->adjustSpeedFraction(-0.1f, dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			{
				myShip->adjustSpeedFraction(1.f, dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{}
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			{
				//updateRoll(dt_sec * freeRoamRollSpeed_radSec);
			}
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			{
				//updateRoll(dt_sec * -freeRoamRollSpeed_radSec);
			}
#if ENABLE_SHIP_CAMERA_DEBUG_TWEAKER
			if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !cameraTweaker)
			{
				cameraTweaker = new_sp<ShipCameraTweakerWidget>(sp_this());
			}
#endif
		}
	}

	void ShipCamera::onActivated()
	{
		static PlayerSystem& playerSys = GameBase::get().getPlayerSystem();

		const std::optional<uint32_t>& myPlayerIdx = getOwningPlayerIndex();
		if (myPlayerIdx.has_value())
		{
			const sp<PlayerBase>& player = playerSys.getPlayer(myPlayerIdx.value());
			if (player)
			{
				player->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_LEFT).addWeakObj(sp_this(), &ShipCamera::handleShootPressed);
			}
		}
	}

	void ShipCamera::onDeactivated()
	{
		static PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
		const std::optional<uint32_t>& myPlayerIdx = getOwningPlayerIndex();
		if (myPlayerIdx.has_value())
		{
			const sp<PlayerBase>& player = playerSys.getPlayer(myPlayerIdx.value());
			if (player)
			{
				player->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_LEFT).removeWeak(sp_this(), &ShipCamera::handleShootPressed);
			}
		}
	}

	void ShipCamera::onMouseWheelUpdate_v(double xOffset, double yOffset)
	{
		//don't dt_sec for zoom controls as we don't want it to slow down with time dilation
		followDistance = glm::clamp<float>(followDistance + float(-yOffset), MIN_FOLLOW, MAX_FOLLOW);
		updateRelativePositioning();
	}

	void ShipCamera::onMouseMoved_v(double xpos, double ypos)
	{
		QuaternionCamera::onMouseMoved_v(xpos, ypos);

		updateRelativePositioning();
		updateShipFacingDirection(); //#suggested visually this looks okay not to do here and let next tick update; it is causing double updates per frame -- which is violating roll speed limits, #prioritized_tickers
		
	}

	void ShipCamera::tick(float dt_sec)
	{
		QuaternionCamera::tick(dt_sec);

		updateShipFacingDirection();

		if (myShip)
		{
			static LevelSystem& levelSys = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSys.getCurrentLevel())
			{
				float world_dt_sec = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();
				worldTimeTicked += world_dt_sec;
				if (bFireHeld && (worldTimeTicked - lastFireTimestamp) > myShip->getFireCooldownSec())
				{
					myShip->fireProjectileInDirection(normalize(getFront()));
					lastFireTimestamp = worldTimeTicked;
				}
			}
		}
		

	}

	void ShipCamera::handleShipTransformChanged(const Transform& xform)
	{
		//be careful here not to update any state on the ship, otherwise this will be a recursive update
		updateRelativePositioning();
	}

	void ShipCamera::handleShootPressed(int state, int modifier_keys)
	{
		if (myShip)
		{
			if (state == GLFW_PRESS)
			{
				bFireHeld = true;
			}
			if (state == GLFW_RELEASE)
			{
				bFireHeld = false;
			}
		}
	}

	void ShipCamera::updateRelativePositioning()
	{
		if (myShip)
		{
			vec3 shipPos = myShip->getWorldPosition();
			vec3 newCamPos = shipPos + (-getFront() * followDistance);
			newCamPos += verticalOffsetFactor * normalize(getUp());
			setPosition(newCamPos);
		}
	}

	void ShipCamera::updateShipFacingDirection()
	{
		if (myShip)
		{
			static GameBase& game = GameBase::get();
			static LevelSystem& levelSystem = game.getLevelSystem();
			static DebugRenderSystem& debugRenderSystem = game.getDebugRenderSystem();
			//uint64_t frame = game.getFrameNumber();

			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				float dt_sec = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();
				vec3 camFront_n = glm::normalize(getFront());
				vec3 shipPos = myShip->getWorldPosition();
				vec3 newShipPos = shipPos + camFront_n * 5.f;

				//calculate viscosity to slow down camera
				vec3 shipDir_n = normalize(myShip->getForwardDir());
				float fwdRelatedness = clamp(dot(shipDir_n, camFront_n), -1.f, 1.f); //[-1,1] range
				//const float VISCOSITY_THRESHOLD = clamp(0.75f, 0.f, 1.f); //dot product is hopefully somwhere near 1, the sharper turn the closer to 0 it will get
				//const float MAX_VISOCITY = 0.90f;
				float viscosity = clamp((fwdRelatedness - VISCOSITY_THRESHOLD) / (1.0f - VISCOSITY_THRESHOLD), 0.f, MAX_VISOCITY);

				//this is likely going to cause an issue when turning with mouse -- we're going to call it twice.
				//but the fix I want requires a prioritized ticker where camera can be ticked near the end of the game loop #TODO
				myShip->moveTowardsPoint(newShipPos, dt_sec, 1.0f, false, 1.0f, viscosity);

				vec3 shipUp_n = normalize(vec3(myShip->getUpDir()));
				vec3 camUp_n = normalize(getUp());
				//float upRelatedness = dot(shipUp_n, camUp_n);

				if (!Utils::vectorsAreSame(shipUp_n, camUp_n))// && upRelatedness < 0.98f)
				{
					vec3 shipRight_n = normalize(myShip->getRightDir());
					vec3 camUp_shipPlane_n = normalize(dot(camUp_n, shipUp_n)*shipUp_n+ dot(camUp_n, shipRight_n)*shipRight_n);

					vec3 rotationAxis = glm::cross(shipUp_n, camUp_shipPlane_n);
					float rotationSign = sign(dot(rotationAxis, camFront_n)); //-1.f, 0.f, or 1.f

					float absRot_rad = glm::acos(clamp(dot(shipUp_n, camUp_shipPlane_n), -1.0f, 1.0f));
					float signedRollSpeed_rad = rollSpeed_rad * rotationSign;

					myShip->roll(signedRollSpeed_rad, dt_sec, absRot_rad);
				}

				//DEBUG
				constexpr bool bEnableDebugRoll = false; //DO NOT LEAVE AS TRUE, will be hard to track down what is doing this
				if constexpr (bEnableDebugRoll) 
				{
					debugRenderSystem.renderLine(shipPos, shipPos + 5.f * shipUp_n, color::red());
					debugRenderSystem.renderLine(shipPos, shipPos + 5.f * camUp_n, color::blue());
				}
			}
		}
	}

	void ShipCameraTweakerWidget::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		LevelSystem& levelSystem = game.getLevelSystem();

		game.getUISystem()->onUIFrameStarted.addWeakObj(sp_this(), &ShipCameraTweakerWidget::handleUIFrameStarted);
	}

	void ShipCameraTweakerWidget::handleUIFrameStarted()
	{
		if (targetCamera)
		{
			ImGuiWindowFlags flags = 0;
			ImGui::Begin("Ship Camera Tweaker!", nullptr, flags);
			{
				ImGui::SliderFloat("MIN_FOLLOW", &targetCamera->MIN_FOLLOW, 0.0f, 40.0f);
				ImGui::SliderFloat("MAX_FOLLOW", &targetCamera->MAX_FOLLOW, 0.0f, 40.0f);
				ImGui::SliderFloat("VISCOSITY_THRESHOLD", &targetCamera->VISCOSITY_THRESHOLD, 0.0f, 1.0f);
				ImGui::SliderFloat("MAX_VISOCITY", &targetCamera->MAX_VISOCITY, 0.0f, 1.0f);
				ImGui::SliderAngle("rollSpeed_rad", &targetCamera->rollSpeed_rad);
				ImGui::SliderFloat("verticalOffsetFactor", &targetCamera->verticalOffsetFactor, 0.0f, 5.0f);
				if (targetCamera->myShip)
				{
					ImGui::SliderFloat("fireCooldownSec", &targetCamera->myShip->fireCooldownSec, 0.01f, 3.f);
				}
				
				if (ImGui::Button("close")) { if(!isPendingDestroy()) destroy(); }
			}
			ImGui::End();
		}
		else if (!isPendingDestroy())
		{
			//#BUG closing window while widget is up appears to cause out of bounds in lp (this object is owned by an lp in ship camera ATOW)
			destroy();
		}
	}

}

