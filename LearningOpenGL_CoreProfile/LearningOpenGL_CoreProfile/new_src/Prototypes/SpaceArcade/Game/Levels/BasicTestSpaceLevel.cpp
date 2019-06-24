#include "BasicTestSpaceLevel.h"
#include <random>
#include "..\SpaceArcade.h"
#include "..\..\Tools\DataStructures\SATransform.h"
#include "..\SAShip.h"
#include "..\..\GameFramework\RenderModelEntity.h"
#include "..\..\GameFramework\SAPlayerBase.h"
#include "..\..\GameFramework\SAPlayerSubsystem.h"
#include "..\..\Rendering\Camera\SACameraBase.h"
#include "..\SAProjectileSubsystem.h"
#include "..\..\GameFramework\Input\SAInput.h"
#include "..\..\GameFramework\SAAssetSubsystem.h"

namespace SA
{
	void BasicTestSpaceLevel::startLevel_v()
	{
		BaseSpaceLevel::startLevel_v();

		SpaceArcade& game = SpaceArcade::get();
		AssetSubsystem& assetSS = game.getAssetSubsystem();

		//specifically not loading model, because assuming model will be owned elsewhere 
		sp<Model3D> carrierModel = assetSS.getModel(game.URLs.carrierURL);
		sp<Model3D> fighterModel = assetSS.getModel(game.URLs.fighterURL);
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
		//cachedSpawnEntities.insert({ carrierShip1.get(), carrierShip1 });

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
			glm::quat rot = glm::angleAxis(startDist(rng_eng), glm::vec3(0, 1, 0)); //angle is a little adhoc, but with radians it should cover full 360 possibilities
			startPos += carrierTransform.position;
			Transform fighterXform = Transform{ startPos, rot, {0.1,0.1,0.1} };
			sp<Ship> fighter = spawnEntity<Ship>(fighterModel, fighterXform, createUnitCubeCollisionInfo());
			//cachedSpawnEntities.insert({ fighter.get(), fighter});
		}

		carrierTransform.position.y += 50;
		carrierTransform.position.x += 120;
		carrierTransform.position.z -= 50;
		carrierTransform.rotQuat = glm::angleAxis(glm::radians(-13.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip2 = spawnEntity<Ship>(carrierModel, carrierTransform, createUnitCubeCollisionInfo());
		//cachedSpawnEntities.insert({ carrierShip2.get(), carrierShip2 });


		if(const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			player->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_LEFT).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleLeftMouseButton);
		}

		sp<Model3D> laserBoltModel = assetSS.getModel(game.URLs.laserURL);
		Transform projectileAABBTransform;
		projectileAABBTransform.scale.z = 4.5;
		laserBoltHandle = game.getProjectileSS()->createProjectileType(laserBoltModel, projectileAABBTransform);
	}

	void BasicTestSpaceLevel::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();

		//using MIter = decltype(cachedSpawnEntities)::iterator;
		//MIter end = cachedSpawnEntities.end();

		////O(n) walk with multimap/set
		//for (MIter current = cachedSpawnEntities.begin(); current != end; ++current)
		//{
		//	if (!current->second.expired())
		//	{
		//		sp<RenderModelEntity> entityToRemove = current->second.lock();
		//		unspawnEntity(entityToRemove);
		//	}
		//}


		BaseSpaceLevel::endLevel_v();
	}

	void BasicTestSpaceLevel::handleLeftMouseButton(int state, int modifier_keys)
	{
		if (state == GLFW_PRESS)
		{
			SpaceArcade& game = SpaceArcade::get();
			if (const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
			{
				if (const sp<CameraBase>& camera = player->getCamera())
				{
					if (!camera->isInCursorMode())
					{
						const sp<ProjectileSubsystem>& projectileSS = game.getProjectileSS();

						glm::vec3 start = camera->getPosition() + glm::vec3(0, -0.25f, 0);
						glm::vec3 direction = camera->getFront();
						projectileSS->spawnProjectile(start, direction, *laserBoltHandle);
					}
				}
			}
		}
	}

}

