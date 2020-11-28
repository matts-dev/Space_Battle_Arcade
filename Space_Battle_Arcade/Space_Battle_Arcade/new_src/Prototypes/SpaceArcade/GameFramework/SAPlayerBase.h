#pragma once
#include "SAGameEntity.h"

#include "..\Tools\DataStructures\MultiDelegate.h"
#include "..\Tools\DataStructures\LifetimePointer.h"
#include "..\Tools\DataStructures\AdvancedPtrs.h"

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
	protected:
		virtual void postConstruct() override;
	public:
		MultiDelegate<const sp<CameraBase>& /*old_camera*/, const sp<CameraBase>& /*new_camera*/> onCameraChanging;
		MultiDelegate< IControllable* /*oldTarget*/, IControllable* /*newTarget*/> onControlTargetSet;
	public:
		/** WARNING: weak bindings should be preferred as the input system will keep strong binding objects alive
				todo: create a GameEntity level call that requests all bindings be cleared, so strong bindings can be cleared in that virtual
		*/
		InputProcessor& getInput() { return *input; }

		void setCamera(const sp<CameraBase>& newCamera);
		const sp<CameraBase>& getCamera() const;

		void setControlTarget(const sp<IControllable>& newControlTarget);
		bool hasControlTarget() const;
		IControllable* getControlTarget();
		sp<IControllable> getControlTargetSP();	//this is more expensive than the raw pointer alternative

		bool isPlayerSimulation() const { return bIsPlayerSimulation; }
		void setIsPlayerSimulation(bool val) { bIsPlayerSimulation = val; }

		int32_t getPlayerIndex() { return myIndex; }
	protected:
		virtual sp<CameraBase> generateDefaultCamera() const = 0;
		virtual void onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget) {}
	public:
		//MultiDelegate<const lp<IControllable>& /*previousTarget*/, const lp<IControllable>& /*newTarget*/> onControlTargetChanging; //#TODO #pair_with_event_AAEF will need virtual inheritance on IControllable for GameEntity or some other mechanism 
	private:
		int32_t myIndex = 0;
		bool bIsPlayerSimulation = false; //some testings may need to create a new player for testing, in this case we don't want to do certain things (like render)
		sp<InputProcessor> input;
		sp<CameraBase> camera;
		fwp<IControllable> controlTarget; //#TODO #releasing_ptr #pair_with_event_AAEF this lifetime pointer, perhaps by having IControllable virtual inherit from game entity
	};
}