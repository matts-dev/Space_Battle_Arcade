#include "ProjectileEditor_Level.h"
#include "..\SpaceArcade.h"
#include "..\SAShip.h"
#include "..\..\GameFramework\RenderModelEntity.h"

namespace SA
{

	void ProjectileEditor_Level::startLevel()
	{
		SpaceArcade& game = SpaceArcade::get();
		sp<Model3D> carrierModel = game.getModel(game.carrierModelKey);

		//TODO delete -- this is temporarily spawning something so we know the level is loaded
		Transform transform;
		transform.position = { 20,0,0 };
		transform.scale = { 1, 1, 1 };
		transform.rotQuat = glm::angleAxis(glm::radians(-33.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip1 = game.spawnEntity<Ship>(carrierModel, transform, createUnitCubeCollisionInfo());

		spawnedModels.insert({ carrierShip1.get(), carrierShip1 });
	}

	void ProjectileEditor_Level::endLevel()
	{
		SpaceArcade& game = SpaceArcade::get();

		using MIter = decltype(spawnedModels)::iterator;
		MIter end = spawnedModels.end();

		//O(n) walk with multimap/set
		for (MIter current = spawnedModels.begin(); current != end; ++current)
		{
			if (!current->second.expired())
			{
				sp<RenderModelEntity> entityToRemove = current->second.lock();
				game.unspawnEntity(entityToRemove);
			}
		}
	}

}

