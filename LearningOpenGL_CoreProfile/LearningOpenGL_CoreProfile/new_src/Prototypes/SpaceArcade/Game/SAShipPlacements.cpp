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
#include "../Tools/SAUtilities.h"
#include "../GameFramework/SADebugRenderSystem.h"
#include "SAProjectileSystem.h"


static constexpr bool bCOMPILE_DEBUG_TURRET = true;

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

	glm::vec3 ShipPlacementEntity::getWorldPosition() const
	{
		//#TODO #scenenodes this may need updating
		if (!cachedWorldPosition.has_value())
		{
			//lazy calculate for efficiency 
			cachedWorldPosition = glm::vec3(cachedModelMat_PxL * glm::vec4(0, 0, 0, 1.f));
		}
		return *cachedWorldPosition;
	}

	glm::vec3 ShipPlacementEntity::getWorldForward_n() const
	{
		using namespace glm;

		if (!cachedWorldForward_n.has_value())
		{
			cachedWorldForward_n = normalize(vec3(cachedModelMat_PxL * vec4(forward_n,0.f)));
		}
		return *cachedWorldForward_n;
	}

	glm::vec3 ShipPlacementEntity::getWorldUp_n() const
	{
		using namespace glm;
		if (!cachedWorldUp_n.has_value())
		{
			cachedWorldUp_n = normalize(vec3(cachedModelMat_PxL * vec4(up_n, 0.f)));
		}
		return *cachedWorldUp_n;
	}

	void ShipPlacementEntity::setTeamData(const TeamData& teamData)
	{
		this->teamData = teamData;
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
		hitLocation = hitLoc;
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
		hitLocation = std::nullopt;
	}

	void ShipPlacementEntity::doDamagedFX()
	{
		using namespace glm;
		if (bHasGeneratorPower)
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
		else //no power generator, do not show shield effect rather show small explosion effect
		{
			glm::vec3 location{ 0.f };
			if (hitLocation.has_value())
			{
				location = *hitLocation;
			}
			else
			{
				//pick a random location
				glm::vec4 perturbedUp = glm::vec4(0, 1, 0, 0);
				float perturbDist = 1.f;
				perturbedUp.x += myRNG->getFloat(-perturbDist, perturbDist);
				perturbedUp.z += myRNG->getFloat(-perturbDist, perturbDist);
				perturbedUp = glm::normalize(cachedModelMat_PxL * perturbedUp);
				location = getWorldPosition() + vec3(perturbedUp);
			}
			ParticleSystem::SpawnParams particleSpawnParams;
			particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
			particleSpawnParams.xform.scale = getTransform().scale;
			particleSpawnParams.xform.position = location;
			particleSpawnParams.xform.scale *= vec3(1.0f); //scale down as to not confuse this with destroying the placement
			GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
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
		//perhaps clear cache function that different functions can call to whipe out all cache
		cachedWorldPosition = std::nullopt;
		cachedWorldForward_n = std::nullopt;
		cachedWorldUp_n = std::nullopt;

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

			hp.current += bHasGeneratorPower ? glm::min(amount / 2, 1) : amount; //half the damage if generators are running
			if (hp.current <= 0.f)
			{
				//destroyed
				if (!isPendingDestroy() && !hasStartedDestructionPhase())
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
				doDamagedFX();
			}
		}

	}

	void ShipPlacementEntity::setForwardLocalSpace(glm::vec3 newForward_ls)
	{
		forward_n = glm::normalize(newForward_ls);
		cachedWorldForward_n = std::nullopt;
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
		spawnXform = localTransform.getModelMatrix();

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

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// turret entity
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void TurretPlacement::tick(float dt_sec)
	{
		using namespace glm;
		static DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();

		if (hasStartedDestructionPhase() || isPendingDestroy())
		{
			return;
		}

		timeSinseFire_sec += dt_sec;

		if (myTarget)
		{
			const vec3 targetPos_wp = myTarget->getWorldPosition();
			const vec3 myWorldPos = getWorldPosition();
			const vec3 toTarget = targetPos_wp - myWorldPos;
			const vec3 toTarget_n = glm::normalize(toTarget);
			const vec3 myForward_wn = getWorldForward_n();
			const vec3 stationaryForward_n = getWorldStationaryForward_n();

			const Transform& xform = getTransform();
			Transform newXform = xform;

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//BOUNDS CHECK -- if target it out of bounds, generate a vector as close as possible to target, but still in bounds.
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			vec3 toTargetInBounds_n = toTarget_n;
			float rotToTargetRaw_Rad = Utils::getRadianAngleBetween(stationaryForward_n, toTargetInBounds_n);
			bool bTargetOutOfBounds = false;
			if (rotToTargetRaw_Rad > rotationLimit_rad)
			{
				vec3 rotAxis_n = glm::normalize(cross(stationaryForward_n, toTargetInBounds_n));
				glm::quat rot = angleAxis(rotationLimit_rad, rotAxis_n);
				toTargetInBounds_n = glm::normalize(rot * stationaryForward_n);
				bTargetOutOfBounds = true;
			}


			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// ROLL TOP OF TURRET -- roll the up direction of ship so that the barrels align with the outer cone
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			static const float ignoreRollRelation = glm::cos(glm::radians<float>(20.f));
			if (bool bVecsDifferent = glm::dot(toTargetInBounds_n, stationaryForward_n) < ignoreRollRelation)
			{
				vec3 myUp_wn = getWorldUp_n();	//wn = world space normal
				if (bool bNotLookingAtFixedOutward = glm::dot(myForward_wn, stationaryForward_n) < 0.99)
				{
					const glm::vec3& stationaryUp_wn = stationaryForward_n; //easier to think about this if we think about the starting forward direction as an up diretion and a circle around that.
					const glm::vec3 targetFrameRight_wn = normalize(cross(stationaryUp_wn, myForward_wn));
					glm::vec3 targetFrameUp_wn = cross(myForward_wn, targetFrameRight_wn); //ortho-normal, do not need to normalize

					vec3 projUpOnTargetFrame_wn = normalize(dot(myUp_wn, targetFrameRight_wn)*targetFrameRight_wn + dot(myUp_wn, targetFrameUp_wn)*targetFrameUp_wn);
					if (bool bVectorUnderUpHorizon = glm::dot(targetFrameUp_wn, projUpOnTargetFrame_wn) < 0.f)
					{
						targetFrameUp_wn *= -1;
					}
					float rotAngle_rad = Utils::getRadianAngleBetween(projUpOnTargetFrame_wn, targetFrameUp_wn);

					constexpr float MAX_ROLL_radsec = glm::radians<float>(30.f);
					constexpr float MIN_ROLL = glm::radians<float>(3.f);
					if (rotAngle_rad > MIN_ROLL)
					{
						float rollThisTick = glm::min(MAX_ROLL_radsec * dt_sec, rotAngle_rad);
						vec3 rotDirCheck = cross(myUp_wn, targetFrameUp_wn); //these vecs should not be same if we're above min roll

						quat rot = glm::angleAxis(dot(rotDirCheck, myForward_wn) > 0.f ? rollThisTick : -rollThisTick, myForward_wn); //roll around our current facing direction
						newXform.rotQuat = rot * newXform.rotQuat;  
					}
					if constexpr (bCOMPILE_DEBUG_TURRET) { debugRenderSystem.renderLine(myWorldPos /*nudge*/ + vec3(0.05f), myWorldPos + projUpOnTargetFrame_wn * 10.f /*nudge*/ + vec3(0.05f), glm::vec3(0.1f, 0.5f, 0.5f)); }
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// UPDATE ROTATION TOWARDS TARGET -- (or as close to target as possible; see above)
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			float angleToTarget_rad = Utils::getRadianAngleBetween(myForward_wn, toTargetInBounds_n);
			constexpr float minRot = glm::radians<float>(2.f);
			if (angleToTarget_rad > minRot)
			{
				float rotThisTick_rad = glm::min(rotationSpeed_radSec * dt_sec, angleToTarget_rad);

				vec3 toTargetAxis = normalize(cross(myForward_wn, toTargetInBounds_n));
				quat rotThisTick_q = angleAxis(rotThisTick_rad, toTargetAxis);
				newXform.rotQuat = rotThisTick_q * newXform.rotQuat;
			}

			setTransform(newXform);

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// fire turret if in focus
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			const ShipPlacementEntity::TeamData& teamData = getTeamData();
			if (glm::dot(myForward_wn, toTarget_n) >= 0.98f 
				&& bShootingEnabled && teamData.primaryProjectile && length2(toTarget_n) > 0.001 && !bTargetOutOfBounds &&
				timeSinseFire_sec >= fireCooldown_sec)
			{
				RNG& rng = getRNG(); float fuzz = 0.5f;//would also be nice if we could get the forward vector of the ship, but that isn't available on target type
				vec3 targetBlurOffset_wv = vec3(rng.getFloat(-fuzz, fuzz), rng.getFloat(-fuzz, fuzz), rng.getFloat(-fuzz, fuzz));//create some fuzzyness around where the turret will shoot.

				barrelIndex = (barrelIndex + 1) % barrelLocations_lp.size();
				vec3 barrelLocation_wp = barrelLocations_lp.size() > 0 ? barrelLocations_lp[barrelIndex] : vec3(0.f);
				barrelLocation_wp = vec3(getParentXLocalModelMatrix() * vec4(barrelLocation_wp, 1.f));

				ProjectileSystem::SpawnData spawnData;
				spawnData.direction_n = glm::normalize((targetPos_wp+targetBlurOffset_wv)- barrelLocation_wp);
				spawnData.start = barrelLocation_wp;// +spawnData.direction_n * 5.0f;
				spawnData.color = teamData.color;
				spawnData.team = teamData.team;
				spawnData.owner = this;

				const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();
				projectileSys->spawnProjectile(spawnData, *teamData.primaryProjectile);
				timeSinseFire_sec = 0.f;
			}

			if constexpr (bCOMPILE_DEBUG_TURRET) 
			{
				debugRenderSystem.renderLine(myWorldPos, myWorldPos + stationaryForward_n * 10.f, glm::vec3(0, 1, 0));
				debugRenderSystem.renderLine(myWorldPos, myWorldPos + toTargetInBounds_n * 10.f, glm::vec3(0, 0, 1));
				debugRenderSystem.renderCone(myWorldPos, stationaryForward_n, rotationLimit_rad, 10.f, glm::vec3(0, 0.5f, 0));
			}
		}
		if constexpr (constexpr bool bDebugBarrelLoc = true)
		{
			mat4 loc1_m = glm::translate(mat4(1.f), barrelLocations_lp[0]);
			mat4 loc2_m = glm::translate(mat4(1.f), barrelLocations_lp[1]);

			mat4 cubeXform = getParentXLocalModelMatrix();// *glm::scale(mat4(1.f), vec3(0.1f));
			debugRenderSystem.renderCube(cubeXform * glm::scale(loc1_m, vec3(0.1f)), vec3(1, 0, 0));
			debugRenderSystem.renderCube(cubeXform * glm::scale(loc2_m, vec3(0.1f)), vec3(1, 0, 0));
		}
	}

	glm::vec3 TurretPlacement::getWorldStationaryForward_n()
{
		using namespace glm;
		if (!cache_worldStationaryForward_n.has_value())
		{
			cache_worldStationaryForward_n = glm::normalize(vec3(getParentXform() * getSpawnXform() * vec4(getLocalForward_n(), 0.f)));
		}
		return *cache_worldStationaryForward_n;
	}

	void TurretPlacement::setTarget(const wp<TargetType>& newTarget)
	{
		myTarget = newTarget;
	}

	void TurretPlacement::updateModelMatrixCache()
	{
		cache_worldStationaryForward_n = std::nullopt;
		Parent::updateModelMatrixCache();
	}

	void TurretPlacement::postConstruct()
	{
		Parent::postConstruct();

		using namespace glm;
		setForwardLocalSpace(glm::vec3(0, 0, 1));

		if (bUseDefaultBarrelLocations)
		{
			barrelLocations_lp.push_back(glm::vec3(0.6f, 0.f, 2.3f));
			barrelLocations_lp.push_back(glm::vec3(-0.6f, 0.f, 2.3));
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
