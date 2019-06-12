#pragma once
#include "SAGameEntity.h"
#include "Interfaces\SATickable.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

namespace SA
{
	class LevelSubsystem;

	/** Base class for a level object */
	class Level : public GameEntity, public Tickable
	{
		friend LevelSubsystem;

	private:
		virtual void startLevel() = 0;
		virtual void endLevel() = 0;
		virtual void tick(float dt_sec) {};

	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) {};
	};
}