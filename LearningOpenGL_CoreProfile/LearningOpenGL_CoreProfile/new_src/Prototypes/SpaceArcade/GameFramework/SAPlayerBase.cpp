#include "SAPlayerBase.h"
#include "Input\SAInput.h"
#include "..\Rendering\Camera\SACameraBase.h"

namespace SA
{
	void PlayerBase::postConstruct()
	{
		input = new_sp<InputProcessor>();
	}

	void PlayerBase::setCamera(const sp<CameraBase>& newCamera)
	{
		onCameraChanging.broadcast(camera, newCamera);
		camera = newCamera;
	}

	const sp<CameraBase>& PlayerBase::getCamera()
	{
		return camera;
	}

}
