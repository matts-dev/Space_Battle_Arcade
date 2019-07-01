#include "BasicTestSpaceLevel.h"

#include <random>
#include <memory>

#include "../SpaceArcade.h"
#include "../SAProjectileSystem.h"
#include "../SAModSystem.h"
#include "../SAUISystem.h"
#include "../SASpawnConfig.h"
#include "../SAShip.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../GameFramework/RenderModelEntity.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SALog.h"

#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../SACollisionDebugRenderer.h"

namespace SA
{
	void BasicTestSpaceLevel::postConstruct()
	{
		SpaceLevelBase::postConstruct(); 

		SpaceArcade& game = SpaceArcade::get();

		if (const sp<ModSystem>& modSystem = game.getModSystem())
		{
			modSystem->onActiveModChanging.addWeakObj(sp_this(), &BasicTestSpaceLevel::handleActiveModChanging);
		}

		collisionDebugRenderer = new_sp<CollisionDebugRenderer>();
	}

	void BasicTestSpaceLevel::startLevel_v()
	{
		SpaceLevelBase::startLevel_v();

		SpaceArcade& game = SpaceArcade::get();
		AssetSystem& assetSS = game.getAssetSystem();

		const sp<UISystem>& uiSystem= game.getUISystem();
		uiSystem->onUIFrameStarted.addStrongObj(sp_this(), &BasicTestSpaceLevel::handleUIFrameStarted);

		//specifically not loading model, because assuming model will be owned elsewhere 
		sp<Model3D> carrierModel = assetSS.getModel(game.URLs.carrierURL);
		sp<Model3D> fighterModel = assetSS.getModel(game.URLs.fighterURL);
		if (!carrierModel || !fighterModel)
		{
			std::cout << "models not available for level" << std::endl;
			return;
		}

		const sp<ModSystem>& modSystem = game.getModSystem();
		sp<Mod> activeMod = modSystem->getActiveMod();
		const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
		if (const auto& iter = spawnConfigs.find("Fighter"); iter != spawnConfigs.end())
		{
			fighterSpawnConfig = iter->second;
		}

		if (!fighterSpawnConfig)
		{
			log("BasicTestSpaceLevel", LogLevel::LOG_ERROR, "Default Spawn Configs not available.");
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
			//sp<Ship> fighter = spawnEntity<Ship>(fighterModel, fighterXform, createUnitCubeCollisionInfo()); //deprecated?
			//sp<Ship> fighter = spawnEntity<Ship>(fighterModel, fighterXform, fighterSpawnConfig->toCollisionInfo()); //also not ideal
			sp<Ship> fighter = spawnEntity<Ship>(fighterSpawnConfig, fighterXform);
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
		laserBoltHandle = game.getProjectileSystem()->createProjectileType(laserBoltModel, projectileAABBTransform);
	}

	void BasicTestSpaceLevel::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		if (const sp<UISystem>& uISystem = game.getUISystem())
		{
			uISystem->onUIFrameStarted.removeStrong(sp_this(), &BasicTestSpaceLevel::handleUIFrameStarted);
		}

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


		SpaceLevelBase::endLevel_v();
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
						const sp<ProjectileSystem>& projectileSys = game.getProjectileSystem();

						glm::vec3 start = camera->getPosition() + glm::vec3(0, -0.25f, 0);
						glm::vec3 direction = camera->getFront();
						projectileSys->spawnProjectile(start, direction, *laserBoltHandle);
					}
				}
			}
		}
	}

	void BasicTestSpaceLevel::handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active)
	{
		
	}

	void BasicTestSpaceLevel::handleUIFrameStarted()
	{
		static SpaceArcade& game = SpaceArcade::get();

		//only draw UI if within cursor mode
		bool bInCursorMode = false;
		if (const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			const sp<CameraBase>& camera = player->getCamera();
			bInCursorMode = camera->isInCursorMode();
		}
		if (!bInCursorMode) { return; }


		ImGui::SetNextWindowPos(ImVec2{ 25, 25 });
		//ImGui::SetNextWindowSize(ImVec2{ 400, 600 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		//flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Space Test Level!", nullptr, flags);
		{
			ImGui::TextWrapped("Debug Variables; if option is not visible it may not be compiled if specific debug macro is not defined. Check defined macros. ");
			ImGui::Separator();
#ifdef SA_RENDER_DEBUG_INFO
			ImGui::Checkbox("Render entity OBB pretests", &bRenderCollisionOBB);
			ImGui::Checkbox("Render entity collision shapes", &bRenderCollisionShapes);
#endif //SA_RENDER_DEBUG_INFO
#ifdef SA_CAPTURE_SPATIAL_HASH_CELLS
			ImGui::Checkbox("Render Spatial Hash Cells", &game.bRenderDebugCells);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
			ImGui::Checkbox("Render Projectile OBBs", &game.bRenderProjectileOBBs);
		}
		ImGui::End();
	}

	void BasicTestSpaceLevel::onEntitySpawned_v(const sp<WorldEntity>& spawned)
	{
		if (sp<Ship> ship = std::dynamic_pointer_cast<Ship>(spawned))
		{
			spawnedShips.insert(ship);
		}
	}

	void BasicTestSpaceLevel::onEntityUnspawned_v(const sp<WorldEntity>& unspawned)
	{
		if (sp<Ship> ship = std::dynamic_pointer_cast<Ship>(unspawned))
		{
			if (auto iter = spawnedShips.find(ship); iter != spawnedShips.end())
			{
				spawnedShips.erase(iter);
			}
		}
	}

	void BasicTestSpaceLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		SpaceLevelBase::render(dt_sec, view, projection);
#ifdef SA_RENDER_DEBUG_INFO
		bool bShouldLoopOverShips = bRenderCollisionOBB || bRenderCollisionShapes;
		if (bShouldLoopOverShips)
		{
			for (const sp<Ship> ship : spawnedShips)
			{
				sp<const ModelCollisionInfo> collisionInfo = ship->getCollisionInfo();
				if (collisionInfo) //TODO this should always be valid, but until I migrate large ships over to new system it may not be valid
				{
					glm::mat4 shipModelMat = ship->getTransform().getModelMatrix();

					if (bRenderCollisionOBB)
					{
						const glm::mat4& aabbLocalXform = collisionInfo->getAABBLocalXform();
						collisionDebugRenderer->renderOBB(shipModelMat, aabbLocalXform, view, projection,
							glm::vec3(0, 0, 1), GL_LINE, GL_FILL);
					}

					if (bRenderCollisionShapes)
					{
						using ConstShapeData = ModelCollisionInfo::ConstShapeData;
						for (const ConstShapeData shapeData : collisionInfo->getConstShapeData())
						{
							collisionDebugRenderer->renderShape(
								shapeData.shapeType,
								shipModelMat * shapeData.localXform,
								view, projection, glm::vec3(1, 0, 0), GL_LINE, GL_FILL
							);
						}
					}

				}
			}
		}

#endif //SA_RENDER_DEBUG_INFO
	}

}

