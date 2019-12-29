#include "SAPlayer.h"
#include "../Rendering/Camera/SACameraFPS.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../GameFramework/SAGameBase.h"

namespace SA
{
	sp<CameraBase> Player::generateDefaultCamera() const
	{
		const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
		sp<CameraBase> defaultCamera = new_sp<CameraFPS>();
		defaultCamera->registerToWindowCallbacks_v(primaryWindow);

		//configure camera to match current camera if there is one
		if (const sp<CameraBase>& currentCamera = getCamera())
		{
			defaultCamera->setPosition(currentCamera->getPosition());
			defaultCamera->lookAt_v(currentCamera->getPosition() + currentCamera->getFront());
		}

		return defaultCamera;
	}
}
