#pragma once
#include <vector>
#include "GameFramework/SAGameEntity.h"
#include "../Game/SASpaceArcadeGlobalConstants.h"
#include "Lights/SADirectionLight.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A globally accessible struct of shared data used for rendering the current scene in a given frame.
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct RenderData : public GameEntity
	{
		RenderData();
		void reset();

		std::vector<DirectionLight> dirLights;
		glm::vec3 renderClearColor{ 0.f };
		glm::vec3 ambientLightIntensity{ 0.f };
		glm::mat4 view{ 1.f }; //note: perhaps it will be better to make a classes that shift their data from frames. So one can just ask the camera for its frame data. No copies required.
		glm::mat4 projection{ 1.f };
		glm::mat4 projection_view{ 1.f };
		std::vector<glm::vec3> playerCamerasPositions = { glm::vec3{0.f} }; //default to a zero position to simply code from having to check array bounds
		float dt_sec = 0.f;
	};
}
