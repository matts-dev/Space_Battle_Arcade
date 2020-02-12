
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
			myShip->onCollided.removeWeak(sp_this(), &ShipCamera::handleCollision);
		}

		myShip = ship;

		if (ship)
		{
			ship->onTransformUpdated.addWeakObj(sp_this(), &ShipCamera::handleShipTransformChanged);
			myShip->onCollided.addWeakObj(sp_this(), &ShipCamera::handleCollision);
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
			bool bCtrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				myShip->setNextFrameBoost(2.0f);
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				constexpr float stopThreshold = 1.0f;
				float currentSpeed = myShip->getSpeed();
				if (bCtrl && currentSpeed < stopThreshold && currentSpeed > -stopThreshold)
				{
					myShip->setSpeedFactor(0.f);
				}
				else
				{
					myShip->adjustSpeedFraction(-0.1f, dt_sec);
				}
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
				rollShip(dt_sec, 1.f);
			}
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			{
				rollShip(dt_sec, -1.f);
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

		if (disableCameraForCollisionTimeRemainingSec > 0.f)
		{
			disableCameraForCollisionTimeRemainingSec -= dt_sec; //currently using engine time
		}
		else
		{
			updateShipFacingDirection();
		}

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

	void ShipCamera::handleCollision()
	{
		disableCameraForCollisionTimeRemainingSec = 5.0f;
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
			vec3 newCamPos = myShip->getWorldPosition() + getCameraOffset();
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
					float signedRollSpeed_rad = shipUpRollSpeed_rad * rotationSign;

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

	void ShipCamera::rollShip(float dt_sec, float direction)
	{
		float deltaCameraRoll = dt_sec * camRollSpeed * direction; 
		if (bUseCrosshairRoll)
		{
			//totally unrealistic, but perhaps okay with small offset distances?
			// force the ship into a position under the rolled camera
			if (myShip)
			{
				float shipRollSlowdownThreshold = myShip->getMaxSpeed() * crosshairRollSlowdownThresholdPerc; //take X% of the max speed and anything under that begins to slow roll
				float shipCurrentSpeed = myShip->getSpeed();

				float rollSlowdownFactor = 1.0f;
				if (bSlowCrosshairRollWithSpeed)
				{
					rollSlowdownFactor = shipCurrentSpeed / (shipRollSlowdownThreshold + 0.001f);
					rollSlowdownFactor -= 0.025f; //clip roll before ship completely stops; this will clip topend too though. Not sure but maybe better than branching?
					rollSlowdownFactor = clamp(rollSlowdownFactor, 0.f, 1.f);
				}
				QuaternionCamera::updateRoll(deltaCameraRoll * rollSlowdownFactor);

				//tried taking vector to ship and transforming that, but it seems to slightly calculate the wrong cam position. Probably a logical error on my end.
				//instead just reversing the camera position calculation produces better results. 
				vec3 newShipPos = getPosition() + -getCameraOffset(); //reverse the camera offset calculation
				Transform transform = myShip->getTransform();
				transform.position = newShipPos;
				myShip->setTransform(transform);
				//handler of transform change will call "updateRelativePositioning"
			}
			else
			{
				QuaternionCamera::updateRoll(deltaCameraRoll);
			}
		}
		else
		{
			QuaternionCamera::updateRoll(deltaCameraRoll);
			updateRelativePositioning();
		}
	}

	glm::vec3 ShipCamera::getCameraOffset()
	{
		vec3 offset = (-getFront() * followDistance);			//go behind ship
		offset += verticalOffsetFactor * normalize(getUp());	//hover above ship
		return offset;
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
				ImGui::SliderFloat("MAX_FOLLOW", &targetCamera->MAX_FOLLOW, 0.0f, 400.0f);
				ImGui::SliderFloat("VISCOSITY_THRESHOLD", &targetCamera->VISCOSITY_THRESHOLD, 0.0f, 1.0f);
				ImGui::SliderFloat("MAX_VISOCITY", &targetCamera->MAX_VISOCITY, 0.0f, 1.0f);
				ImGui::SliderFloat("verticalOffsetFactor", &targetCamera->verticalOffsetFactor, 0.0f, 5.0f);
				ImGui::SliderAngle("shipUpRollSpeed_rad", &targetCamera->shipUpRollSpeed_rad);
				ImGui::SliderAngle("camRollSpeed", &targetCamera->camRollSpeed);
				ImGui::Checkbox("Enable Crosshair Roll", &targetCamera->bUseCrosshairRoll);
				ImGui::SliderFloat("crossRollSlowdownThresholdPerc", &targetCamera->crosshairRollSlowdownThresholdPerc, 0.001f, 0.99f);
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

