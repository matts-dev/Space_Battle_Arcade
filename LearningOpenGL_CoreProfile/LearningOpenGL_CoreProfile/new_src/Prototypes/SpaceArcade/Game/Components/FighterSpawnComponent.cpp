#include "FighterSpawnComponent.h"
#include "../SpaceArcade.h"
#include "../GameSystems/SAModSystem.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../SAShip.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../Tools/SAUtilities.h"

namespace SA
{
	using namespace glm;

	void FighterSpawnComponent::loadSpawnPointData(const SpawnConfig& spawnData)
	{
		mySpawnPoints.clear();
		validSpawnables.clear();

		const std::vector<FighterSpawnPoint>& spawnPnts = spawnData.getSpawnPoints();
		const std::vector<std::string>& spawnableNames = spawnData.getSpawnableConfigsByName();
		if (spawnPnts.size() > 0 || spawnableNames.size() > 0)
		{
			if (!(spawnableNames.size() && spawnPnts.size()))
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Incomplete spawn data, did you forget a spawn point or forget at least 1 named config to spawn at those points?");
			}
			else
			{
				if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
				{
					bool bFoundAtleastOneValidSpawnable = false;

					////////////////////////////////////////////////////////
					// find valid spawnables
					////////////////////////////////////////////////////////
					for (const std::string& configName : spawnableNames)
					{
						const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
						auto findResult = spawnConfigs.find(configName);
						if (findResult != spawnConfigs.end())
						{
							bFoundAtleastOneValidSpawnable = true;

							//this will allow users to enter duplicates, shouldn't break anything but doesn't seem worth it to make sure no duplicates; that could be done at serialization
							validSpawnables.push_back(findResult->second);
						}
					}
					if (!bFoundAtleastOneValidSpawnable)
					{
						log(__FUNCTION__, LogLevel::LOG_ERROR, "No valid spawnable configs found");
					}

					////////////////////////////////////////////////////////
					// cache spawn locations for quick access
					////////////////////////////////////////////////////////
					mySpawnPoints.reserve(spawnPnts.size());
					mySpawnPoints = spawnPnts;
				}
			}
		}
	}

	void FighterSpawnComponent::tick(float dt_sec)
	{
		timeSinceLastSpawnSec += dt_sec;

		if (autoSpawnConfiguration.bEnabled 
			&& bActivated
			&& timeSinceLastSpawnSec > autoSpawnConfiguration.respawnCooldownSec 
			&& spawnedEntities.size() < autoSpawnConfiguration.maxShips)
		{
			spawnEntity();
		}

		//debug spawn locations
		if constexpr (constexpr bool bDebugSpawnPoints = false)
		{
			static DebugRenderSystem& debugRender = GameBase::get().getDebugRenderSystem();
			for (FighterSpawnPoint& spawn : mySpawnPoints)
			{
				vec3 start_wp = vec3(parentXform * vec4(spawn.location_lp, 1.f));
				vec3 dir_wn = normalize(vec3(parentXform*vec4(spawn.direction_ln, 0.f)));
				debugRender.renderRay(dir_wn , start_wp, vec3(1,0,0));
			}
		}
	}

	void FighterSpawnComponent::setPostSpawnCustomization(const PostSpawnCustomizationFunc& inFunc)
	{
		customizationFunc = inFunc;
	}

	void FighterSpawnComponent::postConstruct()
	{
		if(!rng)
		{
			rng = GameBase::get().getRNGSystem().getSeededRNG(13);
		}
	}

	sp<FighterSpawnComponent::SpawnType> FighterSpawnComponent::spawnEntity()
	{
		if (validSpawnables.size() > 0.f && mySpawnPoints.size() > 0.f)
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();

			size_t spawnableIdx = rng->getInt<size_t>(0, validSpawnables.size() - 1);
			if (validSpawnables[spawnableIdx] && currentLevel)
			{
				timeSinceLastSpawnSec = 0.f;

				size_t spawnPntIdx = rng->getInt<size_t>(0, mySpawnPoints.size() - 1);
				FighterSpawnPoint& spawnPoint = mySpawnPoints[spawnPntIdx];

				vec3 spawnPnt_wp = vec3(parentXform * vec4(spawnPoint.location_lp, 1.f));
				vec3 spawnDir_n = normalize(vec3(parentXform * vec4(spawnPoint.direction_ln, 0.f)));

				Ship::SpawnData spawnData;
				spawnData.spawnConfig = validSpawnables[spawnableIdx];
				spawnData.team = teamIdx;
				spawnData.spawnTransform.position = spawnPnt_wp;
				spawnData.spawnTransform.rotQuat = Utils::getRotationBetween(
					normalize(spawnData.spawnConfig->getModelFacingDir_n() * spawnData.spawnConfig->getModelDefaultRotation()),	 //normalizing for safety, shouldn't be necessary analytically, may be necessarily numerically
					spawnDir_n);//spawn ship facing space point dir

				sp<Ship> newEntity = currentLevel->spawnEntity<Ship>(spawnData);
				newEntity->setVelocityDir(spawnDir_n); //this currently normalizes, so normalizing twice but leaving to avoid code fragility

				////////////////////////////////////////////////////////
				// track ship
				////////////////////////////////////////////////////////
				newEntity->onDestroyedEvent->addWeakObj(sp_this(), &FighterSpawnComponent::handleOwnedEntityDestroyed);
				spawnedEntities.insert(newEntity); 

				////////////////////////////////////////////////////////
				// allow post-spawn customization
				////////////////////////////////////////////////////////
				if (customizationFunc)
				{
					//allows user to apply specific things, like a specific brain for example.
					customizationFunc(newEntity);
				}

				newEntity->enterSpawnStasis();

				//customization funciton cannot pass state, if that is needed hook into this delegate and set the state on the newly spawned entity
				onSpawnedEntity.broadcast(newEntity);

				return newEntity;
			}
		}
		return sp<SpawnType>(nullptr);
	}

	void FighterSpawnComponent::handleOwnedEntityDestroyed(const sp<GameEntity>& destroyed)
	{
		//#PROFILE this needs profiling to determine if linear walk is better than using associative container. Therer will be large number of fighters 
		auto findResult = spawnedEntities.find(destroyed);
		if (findResult != spawnedEntities.end())
		{
			spawnedEntities.erase(findResult);
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Owned entity destroyed but could not be found in tracking container, there likely is a logic bug happening!");
		}
	}

	sp<SA::RNG> FighterSpawnComponent::rng = nullptr;

}

