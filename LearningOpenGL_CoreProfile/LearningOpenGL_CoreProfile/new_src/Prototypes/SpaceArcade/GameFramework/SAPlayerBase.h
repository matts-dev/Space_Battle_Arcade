#pragma once
#include "SAGameEntity.h"

#include "..\Tools\DataStructures\MultiDelegate.h"
#include "..\Tools\DataStructures\LifetimePointer.h"

namespace SA
{
	class InputProcessor;
	class CameraBase;
	class IControllable;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The base class representing a player
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class PlayerBase : public GameEntity
	{
	public:
		PlayerBase(int32_t index) : myIndex(index) {}
		
	private:
		virtual void postConstruct() override;

	public:
		/** WARNING: weak bindings should be preferred as the input system will keep strong binding objects alive
				todo: create a GameEntity level call that requests all bindings be cleared, so strong bindings can be cleared in that virtual
		*/
		InputProcessor& getInput() { return *input; }

		/** Camera related */
		void setCamera(const sp<CameraBase>& newCamera);
		const sp<CameraBase>& getCamera() const;
		MultiDelegate<const sp<CameraBase>& /*old_camera*/, const sp<CameraBase>& /*new_camera*/> onCameraChanging;
		void setControlTarget(const sp<IControllable>& newControlTarget);
		bool hasControlTarget() const;

	protected:
		virtual sp<CameraBase> generateDefaultCamera() const = 0;
	public:
		//MultiDelegate<const lp<IControllable>& /*previousTarget*/, const lp<IControllable>& /*newTarget*/> onControlTargetChanging; //#TODO will need virtual inheritance on IControllable for GameEntity
	private:
		int32_t myIndex = 0;
		sp<InputProcessor> input;
		sp<CameraBase> camera;
		sp<IControllable> controlTarget;
	};
}