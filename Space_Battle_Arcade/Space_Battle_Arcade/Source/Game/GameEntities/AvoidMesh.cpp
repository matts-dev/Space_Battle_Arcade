#include "AvoidMesh.h"

#include "Game/AssetConfigs/SASpawnConfig.h"
#include "GameFramework/Components/CollisionComponent.h"
#include "GameFramework/SACollisionUtils.h"
#include "GameFramework/SALevel.h"
#include "Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"

namespace SA
{
	static bool bRenderAvoidanceSpheres = false;
	void AvoidMesh::setRenderAvoidanceSpheres(bool bRender)
	{
		bRenderAvoidanceSpheres = bRender;
	}

	AvoidMesh::AvoidMesh(const SpawnData& spawnData) :
		Parent(spawnData.spawnConfig->getModel(), spawnData.spawnTransform),
		collisionData(spawnData.spawnConfig->toCollisionInfo())
	{
		spawnConfig = spawnData.spawnConfig;

		bEditorMode = spawnData.bEditorMode;
		if (!bEditorMode)
		{
			CollisionComponent* collisionComp = createGameComponent<CollisionComponent>();
			collisionComp->setCollisionData(collisionData);
			collisionComp->setKinematicCollision(spawnData.spawnConfig->requestCollisionTests());
		}
	}

	void AvoidMesh::postConstruct()
	{
		Parent::postConstruct();

		for (const AvoidanceSphereSubConfig& sphereConfig : spawnConfig->getAvoidanceSpheres())
		{
			//must wait for postConstruct because we need to pass sp_this() for owner field.
			sp<AvoidanceSphere> avoidanceSphere = new_sp<AvoidanceSphere>(sphereConfig.radius, sphereConfig.localPosition, sp_this());
			avoidanceSphere->setParentScalesRadius(true); //asteroids are tiny and scaled up, we need radius to respect scale.
			avoidanceSpheres.push_back(avoidanceSphere); 
		}

		if (!bEditorMode)
		{
			updateAvoidanceSpheres();
		
			//WARNING: caching world sp will create cyclic reference
			if (LevelBase* world = getWorld())
			{
				collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB());
			}
			updateCollision();
		}
	}

	void AvoidMesh::render(Shader& shader)
	{
		const std::vector<TeamData>& teams = spawnConfig->getTeams();
		glm::vec3 teamColor = teams.size() > 0 ? teams[0].teamTint : glm::vec3(1.f);
		shader.setUniform3f("objectTint", teamColor);

		Parent::render(shader);

		if (avoidanceSpheres.size() > 0 && bRenderAvoidanceSpheres)
		{
			for (sp<AvoidanceSphere>& avoidSphere : avoidanceSpheres)
			{
				avoidSphere->render();
			}
		}
	}

	void AvoidMesh::setTransform(const Transform& inTransform)
	{
		Parent::setTransform(inTransform);
		updateAvoidanceSpheres();
		updateCollision();
	}

	void AvoidMesh::updateAvoidanceSpheres()
	{
		if (bEditorMode) { return; }

		glm::mat4 astroidModelMat = getTransform().getModelMatrix();

		for (sp<AvoidanceSphere>& avoidanceSphere : avoidanceSpheres)
		{
			avoidanceSphere->setParentXform(astroidModelMat);
		}
	}

	void AvoidMesh::updateCollision()
	{
		if (!bEditorMode)
		{
			Transform xform = getTransform();
			collisionData->updateToNewWorldTransform(xform.getModelMatrix());

			LevelBase* world = getWorld();
			if (world && collisionHandle)
			{
				SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
				worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());
			}
		}
	}

}

