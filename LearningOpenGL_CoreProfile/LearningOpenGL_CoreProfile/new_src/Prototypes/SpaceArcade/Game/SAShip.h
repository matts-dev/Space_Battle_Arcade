#pragma once
#include "..\GameFramework\SAWorldEntity.h"
#include "..\Tools\ModelLoading\SAModel.h"
#include "..\GameFramework\RenderModelEntity.h"
#include "SACollisionSubsystem.h"


namespace SA
{
	class Ship : public RenderModelEntity
	{
	public:
		Ship(
			const sp<Model3D>& model,
			const Transform& spawnTransform,
			const sp<ModelCollisionInfo>& inCollisionData
			);

	protected:
		virtual void postConstruct() override;
		virtual void tick(float deltatime) override;

	private:
		const std::array<glm::vec4, 8> getWorldOBB(const glm::mat4 xform) const;

	private:
		//helper data structures
		std::vector<sp<SH::GridNode<GameEntity>>> overlappingNodes_SH;

	private:
		sp<CollisionSubsystem> CollisionSS;

	private:
		const sp<ModelCollisionInfo> collisionData;
		up<SH::HashEntry<GameEntity>> collisionHandle = nullptr;

		glm::vec3 velocity;
	};
}

