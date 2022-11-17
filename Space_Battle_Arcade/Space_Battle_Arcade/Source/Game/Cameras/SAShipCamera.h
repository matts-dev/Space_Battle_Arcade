#pragma once
#include <detail/func_trigonometric.hpp>
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../Tools/DataStructures/LifetimePointer.h"
#include "../../Tools/DataStructures/AdvancedPtrs.h"
#include "../Environment/StarJumpData.h"
namespace SA
{
	class Ship;
	class ShipCameraTweakerWidget;
	class CollisionData;
	struct Transform;

#define ENABLE_SHIP_CAMERA_DEBUG_TWEAKER 1

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A camera that pilots a ship
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShipCamera : public QuaternionCamera
	{
	public:
		void followShip(const sp<Ship>& ship);
		virtual glm::mat4 getView() const override;
		void enableStarJump(bool bEnable, bool bSkipTransition = false);
	protected:
		virtual void postConstruct() override;
	private:
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset) override;
		virtual void onMouseMoved_v(double xpos, double ypos) override;
		virtual void tick(float dt_sec) override;
		virtual void tickKeyboardInput(float dt_sec) override;
		virtual void onActivated() override;
		virtual void onDeactivated() override;
		virtual glm::vec3 getVelocity() const override;
		void handleShipTransformChanged(const Transform& xform);
		void handleCollision();
		void handleShootPressed(int state, int modifier_keys);
		void handlePostGameLoop(float dt_sec);
		void updateRelativePositioning();
		glm::vec3 getCameraOffset();
		void updateShipFacingDirection();
		void rollShip(float dt_sec, float direction);
	private:
#if ENABLE_SHIP_CAMERA_DEBUG_TWEAKER
		friend class ShipCameraTweakerWidget;
		lp<ShipCameraTweakerWidget> cameraTweaker;
#endif
	private: //controlling parameters 
		float MIN_FOLLOW = 1.55f;
		float MAX_FOLLOW = 30.f;
		float VISCOSITY_THRESHOLD = 0.75f; //should be in range [0, 1] as viscosity is in that range
		float MAX_VISOCITY = 0.90f;		//should be in range [0, 1]
		float shipUpRollSpeed_rad = glm::radians(180.0f);
		float camRollSpeed = glm::radians(180.0f);
		float verticalOffsetFactor = 0.5f;
		float crosshairRollSlowdownThresholdPerc = 0.25f;
	private:
		glm::vec3 collisionAdjustedPosition{ 0.f }; //this should update at the end of every gameloop tick
		float worldTimeTicked = 0.f;
		float lastFireTimestamp = 0.f;
		float disableCameraForCollisionTimeRemainingSec = 0.f;
		bool bUseCrosshairRoll = true;
		bool bSlowCrosshairRollWithSpeed = true;
		bool bFireHeld = false;
		bool bAltPressed = false;
	private:
		StarJumpData sj;
		uint32_t owningPlayerIndex = 0;
		sp<CollisionData> collisionData;
		lp<Ship> myShip;
		float followDistance = 10.f;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A widget for tweaking values on camera in real time
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShipCameraTweakerWidget final : public GameEntity
	{
	public:
		ShipCameraTweakerWidget(const sp<ShipCamera>& shipCamera) 
			: targetCamera(shipCamera) 
		{};
	private:
		virtual void postConstruct() override;
		void handleUIFrameStarted();
	private:
		fwp<ShipCamera> targetCamera;
	};
}