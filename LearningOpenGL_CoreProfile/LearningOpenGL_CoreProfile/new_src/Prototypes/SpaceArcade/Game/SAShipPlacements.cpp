#include "SAShipPlacements.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "AssetConfigs/SAConfigBase.h"
#include "../GameFramework/SACollisionUtils.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../GameFramework/Components/CollisionComponent.h"
#include "OptionalCompilationMacros.h"
#include "../Rendering/RenderData.h"
#include "../GameFramework/SARenderSystem.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "../GameFramework/SAParticleSystem.h"
#include "FX/SharedGFX.h"
#include "../GameFramework/SATimeManagementSystem.h"
#include "../GameFramework/SARandomNumberGenerationSystem.h"

namespace SA
{
	static const std::string comStr = "COMMUNICATIONS";
	static const std::string turretStr = "TURRET";
	static const std::string defStr = "DEFENSE";
	static const std::string invalidStr = "INVALID";

	std::string lexToString(PlacementType enumValue)
	{
		switch (enumValue)
		{
			case PlacementType::COMMUNICATIONS: return comStr;
			case PlacementType::DEFENSE: return defStr;
			case PlacementType::TURRET: return turretStr;
			default: return invalidStr;
		}
	}

	SA::PlacementType stringToLex(const std::string& strValue)
	{
		if (strValue == comStr)
		{
			return PlacementType::COMMUNICATIONS;
		}
		else if (strValue == turretStr)
		{
			return PlacementType::TURRET;
		}
		else if (strValue == defStr)
		{
			return PlacementType::DEFENSE;
		}

		return PlacementType::INVALID;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ship placement entity
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void ShipPlacementEntity::postConstruct()
	{
		Parent::postConstruct();

		TeamComponent* teamComp = createGameComponent<TeamComponent>();
		HitPointComponent* hpComp = createGameComponent<HitPointComponent>();

		if (hpComp)
		{
			hpComp->hp.current = 100;
			hpComp->hp.max = 100;
		}

		myRNG = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
		destroyAtSec += myRNG->getFloat(0.f, 1.f);//create varaible time for destruction 
	}

	void ShipPlacementEntity::onDestroyed()
	{
		Parent::onDestroyed();

		//remove from spatial hash since we're allowing this to persist even while destroyed
		collisionHandle = nullptr;
	}

	//void ShipPlacementEntity::tick(float dt_sec)
	//{
	//	Parent::tick(dt_sec);
	//}

	void ShipPlacementEntity::draw(Shader& shader)
	{
		if (!isPendingDestroy())
		{
			if (getModel())
			{
				shader.setUniformMatrix4fv(modelMatrixUniform.c_str(), 1, GL_FALSE, glm::value_ptr(cachedModelMat_PxL));
				RenderModelEntity::draw(shader);
			}
#if SA_RENDER_DEBUG_INFO
			if (collisionData)
			{
				GameBase& engine = GameBase::get();
				static RenderSystem& renderSystem = engine.getRenderSystem();

				if (const RenderData* frd = renderSystem.getFrameRenderData_Read(engine.getFrameNumber()))
				{
					collisionData->debugRender(cachedModelMat_PxL, frd->view, frd->projection);
				}
			}
#endif //SA_RENDER_DEBUG_INFO
		}
	}

	void ShipPlacementEntity::setParentXform(glm::mat4 parentXform)
	{
		this->parentXform = parentXform;

		updateModelMatrixCache();

		//TODO_update_collision;
	}

	void ShipPlacementEntity::setTransform(const Transform& inTransform)
	{
		RenderModelEntity::setTransform(inTransform);

		updateModelMatrixCache();
	}

	void ShipPlacementEntity::notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{
		if (const TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			if (teamComp->getTeam() != hitProjectile.team)
			{
				//we were hit by a non-team member, apply game logic!
				adjustHP(-hitProjectile.damage);
			}
			else
			{
				//play friendly fire fx?
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "ship component hit without being assigned a team component");
		}
	}

	void ShipPlacementEntity::doShieldFX()
	{
		if (activeShieldEffect)
		{
			activeShieldEffect->resetTimeAlive();
		}
		else
		{
			if (const sp<Model3D>& myModel = getMyModel())
			{
				ParticleSystem::SpawnParams particleSpawnParams;
				particleSpawnParams.particle = SharedGFX::get().shieldEffects_ModelToFX->getEffect(myModel, ShieldColor());

				particleSpawnParams.parentXform = cachedModelMat_PxL;
				particleSpawnParams.xform.scale;

				activeShieldEffect = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
			}
		}
	}

	void ShipPlacementEntity::tickDestroyingFX()
	{
		static ParticleSystem& particleSys = GameBase::get().getParticleSystem();

		auto makeDefaultExplosion = [this]() {
			ParticleSystem::SpawnParams particleSpawnParams;
			particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
			particleSpawnParams.xform.position = cachedModelMat_PxL * glm::vec4(0, 0, 0, 1.f);
			particleSpawnParams.xform.scale = getTransform().scale;
			return particleSpawnParams;
		};
		auto sharedExplosionLogic = [this, makeDefaultExplosion](ParticleSystem& particleSys) 
		{
			ParticleSystem::SpawnParams particleSpawnParams = makeDefaultExplosion();
			glm::vec4 perturbedUp = glm::vec4(0, 1, 0, 0);
			float perturbDist = 1.f;
			perturbedUp.x += myRNG->getFloat(-perturbDist, perturbDist);
			perturbedUp.z += myRNG->getFloat(-perturbDist, perturbDist);
			perturbedUp = glm::normalize(cachedModelMat_PxL * perturbedUp);
			if (bool bUseVelocity = myRNG->getFloat(0.f, 1.f) > 0.5f)
			{
				particleSpawnParams.velocity = perturbedUp * myRNG->getFloat(1.f, 10.f);
			}
			else //static particle
			{
				particleSpawnParams.xform.position += glm::vec3(perturbedUp * myRNG->getFloat(1.f, 5.f)); //scale the perturbed up for some explosion position
			}
			particleSpawnParams.xform.scale *= glm::vec3((myRNG->getFloat<float>(4.f, 10.f)));
			particleSys.spawnParticle(particleSpawnParams);
		};

		currentDestrutionPhaseSec += destructionTickFrequencySec;

		float completePerc = currentDestrutionPhaseSec / destroyAtSec;

		//before 50% have fewer explosions
		if (completePerc < 0.5f)
		{
			if (bool bShouldExplode = (myRNG->getFloat(0.f, 1.0f) < 0.5f))
			{
				sharedExplosionLogic(particleSys);
			}
		}
		else if (completePerc < 0.9f)
		{
			if (myRNG->getFloat(0.f, 1.0f) < 0.2f)
			{
				sharedExplosionLogic(particleSys);
			}
		}
		else if (completePerc >= 1.0f)
		{
			getWorld()->getWorldTimeManager()->removeTimer(destructionTickDelegate);		//stop the timer, it is time to destroy

			ParticleSystem::SpawnParams particleSpawnParams = makeDefaultExplosion();
			particleSpawnParams.xform.scale = getTransform().scale * glm::vec3(20.f);

			particleSys.spawnParticle(particleSpawnParams);
			destroy();
		}
	}

	void ShipPlacementEntity::updateModelMatrixCache()
	{
		cachedModelMat_PxL = parentXform * getModelMatrix();

		if (collisionData)
		{
			collisionData->updateToNewWorldTransform(cachedModelMat_PxL);

			LevelBase* world = getWorld();
			if (world && collisionHandle)
			{
				SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
				worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());
			}
		}

		if (activeShieldEffect)
		{
			activeShieldEffect->parentXform_m = cachedModelMat_PxL; //use the complete transform as parent transform, add a little scale to the particle itself
		}
	}

	void ShipPlacementEntity::adjustHP(int amount)
	{
		if (HitPointComponent* hpComp = getGameComponent<HitPointComponent>())
		{
			HitPoints& hp = hpComp->hp;

			hp.current += amount;
			if (hp.current <= 0.f)
			{
				//destroyed
				bool bStartedDestruction = destructionTickDelegate != nullptr;
				if (!isPendingDestroy() && !bStartedDestruction)
				{
					if (LevelBase* world = getWorld())
					{
						destructionTickDelegate = new_sp<MultiDelegate<>>();
						destructionTickDelegate->addWeakObj(sp_this(), &ShipPlacementEntity::tickDestroyingFX);
						world->getWorldTimeManager()->createTimer(destructionTickDelegate, destructionTickFrequencySec, true);
					}
					else
					{
						destroy();	//can't get level? just skip and destroy
					}
				}
			}
			else
			{
				//damaged
				doShieldFX();
			}
		}

	}

	void ShipPlacementEntity::replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig)
	{
		bool bModelsAreDifferent = config.relativeFilePath != newConfig.relativeFilePath;
		bool bCollisionModelsDifferent = config.relativeCollisionModelFilePath != newConfig.relativeCollisionModelFilePath;

		config = newConfig;

		////////////////////////////////////////////////////////
		// static local positioning
		////////////////////////////////////////////////////////
		Transform localTransform;
		localTransform.position = config.position;
		localTransform.rotQuat = getRotQuatFromDegrees(config.rotation_deg);
		localTransform.scale = config.scale;
		setTransform(localTransform);

		updateModelMatrixCache();

		////////////////////////////////////////////////////////
		// visual model3d
		////////////////////////////////////////////////////////
		if (bModelsAreDifferent)
		{
			sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(config.getFullPath(owningConfig));
			replaceModel(model);
		}

		////////////////////////////////////////////////////////
		// collision
		////////////////////////////////////////////////////////
		if (const sp<const Model3D>& placementModel3D = getModel())
		{
			CollisionShapeFactory& shapeFactory = SpaceArcade::get().getCollisionShapeFactoryRef();

			collisionData = new_sp<CollisionData>();
			collisionData->setAABBtoModelBounds(*placementModel3D);

			bool bUseModelAABBCollision = config.relativeCollisionModelFilePath.length() == 0;
			if (!bUseModelAABBCollision)
			{
				//use custom collision shape
				std::string fullModelCollisionPath = config.getCollisionModelFullPath(owningConfig);

				if (sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(fullModelCollisionPath))
				{
					CollisionData::ShapeData newShapeInfo;
					newShapeInfo.localXform = glm::mat4(1.f);
					newShapeInfo.shape = shapeFactory.generateShape(ECollisionShape::MODEL, fullModelCollisionPath);
					newShapeInfo.shapeType = ECollisionShape::MODEL;
					collisionData->addNewCollisionShape(newShapeInfo);
				}
				else
				{
					//fallback generate using AABB
					bUseModelAABBCollision = true;
				}
			}

			//model loading may fail, so as a fallback use the AABB; mean this should not be an else statement on the model loading branch above
			if (bUseModelAABBCollision)
			{
				//use model bounding box of the entire model since no specific shape is provided
				CollisionData::ShapeData newShapeInfo;
				newShapeInfo.localXform = collisionData->getAABBLocalXform();
				newShapeInfo.shape = shapeFactory.generateShape(ECollisionShape::CUBE);
				newShapeInfo.shapeType = ECollisionShape::CUBE;
				collisionData->addNewCollisionShape(newShapeInfo);
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "loading placement config without a model to base AABB collision - no collision data will be generated for this placement");
		}
		collisionHandle = nullptr;

		if (collisionData)
		{
			CollisionComponent* collComp = getGameComponent<CollisionComponent>();
			if (!collComp)
			{
				//deferring creation of collision component until we have valid collision; that way if modder somehow fails to provide collision data things we won't have a collision component unless there's valid collision
				collComp = createGameComponent<CollisionComponent>();
			}
			collComp->setCollisionData(collisionData);
			collComp->setKinematicCollision(false); //don't attempt to collide with owner

			if (LevelBase* world = getWorld()) //WARNING: caching world sp will create cyclic reference
			{
				updateModelMatrixCache(); //force update of collision data in case there are any logic changes
				collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB());
			}
			else
			{
				throw std::logic_error("Attempting to configure ship placement but no world available");
			}
		}
	}
}

std::string SA::PlacementSubConfig::getFullPath(const ConfigBase& owningConfig) const
{
	return owningConfig.getOwningModDir() + relativeFilePath;
}

std::string SA::PlacementSubConfig::getCollisionModelFullPath(const ConfigBase& owningConfig) const
{
	if (relativeCollisionModelFilePath.length() > 0)
	{
		return owningConfig.getOwningModDir() + relativeCollisionModelFilePath;
	}
	else
	{
		//just return empty string
		return relativeCollisionModelFilePath;
	}
}
