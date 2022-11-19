#include "Rendering/RenderData.h"

#include "GameFramework/SAGameBase.h"

namespace SA
{
	RenderData::RenderData()
	{
		//dirLights.reserve(GameBase::getConstants().MAX_DIR_LIGHTS);
		dirLights.resize(GameBase::getConstants().MAX_DIR_LIGHTS);
		reset();
	}

	void RenderData::reset()
	{
		for (size_t idx = 0; idx < GameBase::getConstants().MAX_DIR_LIGHTS; ++idx)
		{
			dirLights[idx] = DirectionLight{};
		}
		view = glm::mat4{ 1.f };
		projection = glm::mat4{ 1.f };
		ambientLightIntensity = glm::vec3{ 0.f };
		playerCamerasPositions.resize(1); //shrink to 1 player camera
		playerCamerasPositions[0] = glm::vec3(0.f); //zero out the camera
		dt_sec = 0.f;
		renderClearColor = glm::vec3(0.f);
	}

}

