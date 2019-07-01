#pragma once
#include "SAGameEntity.h"

#include "..\Tools\DataStructures\MultiDelegate.h"

namespace SA
{
	class InputProcessor;
	class CameraBase;

	class PlayerBase : public GameEntity
	{
	private:
		virtual void postConstruct() override;

	public:
		/** WARNING: weak bindings should be preferred as the input system will keep strong binding objects alive
				todo: create a GameEntity level call that requests all bindings be cleared, so strong bindings can be cleared in that virtual
		*/
		InputProcessor& getInput() { return *input; }

		/** Camera related */
		void setCamera(const sp<CameraBase>& newCamera);
		const sp<CameraBase>& getCamera();
		MultiDelegate<const sp<CameraBase>& /*old_camera*/, const sp<CameraBase>& /*new_camera*/> onCameraChanging;


	private:
		sp<InputProcessor> input;
		sp<CameraBase> camera;
	};
}