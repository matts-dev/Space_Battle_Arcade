#include "SAPlayerBase.h"
#include "Input/SAInput.h"
#include "Interfaces/SAIControllable.h"
#include "../Rendering/Camera/SACameraBase.h"

namespace SA
{
	void PlayerBase::postConstruct()
	{
		input = new_sp<InputProcessor>();
	}

	void PlayerBase::setCamera(const sp<CameraBase>& newCamera)
	{
		onCameraChanging.broadcast(camera, newCamera);
		if (camera)
		{
			camera->deactivate();
			camera->setOwningPlayerIndex({});
		}
		camera = newCamera;
		if (newCamera)
		{
			camera->setOwningPlayerIndex(myIndex);
			newCamera->activate();
		}
	}

	const sp<CameraBase>& PlayerBase::getCamera() const
	{
		return camera;
	}

	void PlayerBase::setControlTarget(const sp<IControllable>& newControlTarget)
	{
		if (controlTarget.get() != newControlTarget.get())
		{
			//onControlTargetChanging.broadcast(controlTarget, newControlTarget);

			if (controlTarget)
			{
				controlTarget->onPlayerControlReleased();
			}

			controlTarget = newControlTarget;

			if (newControlTarget)
			{
				newControlTarget->onPlayerControlTaken();
				setCamera(newControlTarget->getCamera());
			}
			else
			{
				setCamera(generateDefaultCamera());
			}
		}
	}

	bool PlayerBase::hasControlTarget() const
	{
		return bool(controlTarget);
	}


}
