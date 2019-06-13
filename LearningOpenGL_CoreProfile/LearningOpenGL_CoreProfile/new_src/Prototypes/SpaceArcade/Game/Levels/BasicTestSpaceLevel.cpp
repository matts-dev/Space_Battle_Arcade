#include "BasicTestSpaceLevel.h"
#include <random>
#include "..\SpaceArcade.h"
#include "..\..\Tools\DataStructures\SATransform.h"
#include "..\SAShip.h"
#include "..\..\GameFramework\RenderModelEntity.h"

namespace SA
{
	void BasicTestSpaceLevel::startLevel_v()
	{
		BaseLevel::startLevel_v();

		SpaceArcade& game = SpaceArcade::get();

		sp<Model3D> carrierModel = game.getModel(game.carrierModelKey);
		sp<Model3D> fighterModel = game.getModel(game.fighterModelKey);
		if (!carrierModel || !fighterModel)
		{
			std::cout << "models not available for level" << std::endl;
			return;
		}


		Transform carrierTransform;
		carrierTransform.position = { 200,0,0 };
		carrierTransform.scale = { 5, 5, 5 };
		carrierTransform.rotQuat = glm::angleAxis(glm::radians(-33.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip1 = spawnEntity<Ship>(carrierModel, carrierTransform, createUnitCubeCollisionInfo());
		cachedSpawnEntities.insert({ carrierShip1.get(), carrierShip1 });

		std::random_device rng;
		std::seed_seq seed{ 28 };
		std::mt19937 rng_eng = std::mt19937(seed);
		std::uniform_real_distribution<float> startDist(-200.f, 200.f); //[a, b)

		int numFighterShipsToSpawn = 5000;
#ifdef _DEBUG
		numFighterShipsToSpawn = 500;
#endif//NDEBUG 
		for (int fighterShip = 0; fighterShip < numFighterShipsToSpawn; ++fighterShip)
		{
			glm::vec3 startPos(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));
			glm::quat rot = glm::angleAxis(startDist(rng_eng), glm::vec3(0, 1, 0)); //angle is a little addhoc, but with radians it should cover full 360 possibilities
			startPos += carrierTransform.position;
			Transform fighterXform = Transform{ startPos, rot, {0.1,0.1,0.1} };
			sp<Ship> fighter = spawnEntity<Ship>(fighterModel, fighterXform, createUnitCubeCollisionInfo());
			cachedSpawnEntities.insert({ fighter.get(), fighter});
		}

		carrierTransform.position.y += 50;
		carrierTransform.position.x += 120;
		carrierTransform.position.z -= 50;
		carrierTransform.rotQuat = glm::angleAxis(glm::radians(-13.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip2 = spawnEntity<Ship>(carrierModel, carrierTransform, createUnitCubeCollisionInfo());
		cachedSpawnEntities.insert({ carrierShip2.get(), carrierShip2 });

	}

	void BasicTestSpaceLevel::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();

		using MIter = decltype(cachedSpawnEntities)::iterator;
		MIter end = cachedSpawnEntities.end();

		//O(n) walk with multimap/set
		for (MIter current = cachedSpawnEntities.begin(); current != end; ++current)
		{
			if (!current->second.expired())
			{
				sp<RenderModelEntity> entityToRemove = current->second.lock();
				unspawnEntity(entityToRemove);
			}
		}


		BaseLevel::endLevel_v();
	}

}

