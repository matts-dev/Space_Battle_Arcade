#include "SAShipPlacements.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAAssetSystem.h"
#include "Tools/ModelLoading/SAModel.h"
#include "AssetConfigs/SAConfigBase.h"
#include "GameFramework/SACollisionUtils.h"
#include "SpaceArcade.h"
#include "GameFramework/SALevel.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "GameFramework/Components/CollisionComponent.h"
#include "OptionalCompilationMacros.h"
#include "../Rendering/RenderData.h"
#include "GameFramework/SARenderSystem.h"
#include "GameFramework/Components/GameplayComponents.h"
#include "GameFramework/SAParticleSystem.h"
#include "FX/SharedGFX.h"
#include "../GameFramework/SATimeManagementSystem.h"
#include "GameFramework/SARandomNumberGenerationSystem.h"
#include "Tools/SAUtilities.h"
#include "GameFramework/SADebugRenderSystem.h"
#include "GameSystems/SAProjectileSystem.h"
#include "Tools/color_utils.h"
#include "Rendering/OpenGLHelpers.h"
#include "GameModes/ServerGameMode_SpaceBase.h"
#include "Levels/SASpaceLevelBase.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAAudioSystem.h"


static constexpr bool bCOMPILE_DEBUG_TURRET = false;
static constexpr bool bCOMPILE_DEBUG_SATELLITE = false;
static constexpr float variabilityInTargetWaitSec = 5.f;

namespace SA
{
	static const std::string comStr = "COMMUNICATIONS";
	static const std::string turretStr = "TURRET";
	static const std::string defStr = "DEFENSE";
	static const std::string invalidStr = "INVALID";


	/*static*/ sp<Model3D> CommunicationPlacement::seekerModel = nullptr;
	/*static*/ sp<Shader> CommunicationPlacement::seekerShader = nullptr;
	/*static*/ uint32_t CommunicationPlacement::tessellatedTextureID = 0;

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
			HitPoints hp;
			hp.current = 100.f;
			hp.max = 100.f;
			hpComp->overwriteHP(hp);
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

	void ShipPlacementEntity::render(Shader& shader)
	{
		if (!isPendingDestroy())
		{
			if (getModel())
			{
				shader.setUniformMatrix4fv(modelMatrixUniform.c_str(), 1, GL_FALSE, glm::value_ptr(cachedModelMat_PxL));
				RenderModelEntity::render(shader);
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
			cachedWorldForward_n = normalize(vec3(cachedModelMat_PxL * vec4(forward_ln,0.f)));
		}
		return *cachedWorldForward_n;
	}

	glm::vec3 ShipPlacementEntity::getWorldUp_n() const
	{
		using namespace glm;
		if (!cachedWorldUp_n.has_value())
		{
			cachedWorldUp_n = normalize(vec3(cachedModelMat_PxL * vec4(up_ln, 0.f)));
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
				notifyDamagingHit(hitProjectile, hitLoc);
				adjustHP(-float(hitProjectile.damage), hitProjectile.owner);
				//WARNING: anything after adjusting HP make cause this object to be destroyed, do not make calls after it. #nextengine all destroys happen 1 frame deferred
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
					float hdrFactor = GameBase::get().getRenderSystem().isUsingHDR() ? 3.f : 1.f; //@hdr_tweak

					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = SharedGFX::get().shieldEffects_ModelToFX->getEffect(myModel, ShieldColor() * hdrFactor);

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
			//particleSpawnParams.xform.scale = getTransform().scale;
			particleSpawnParams.xform.position = location;
			particleSpawnParams.xform.scale *= vec3(0.1f); //scale down as to not confuse this with destroying the placement
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
			//particleSpawnParams.xform.scale = getTransform().scale;
			//if (Ship* carrierShip = weakOwner.isValid() ? weakOwner.fastGet() : nullptr)
			//{
			//	particleSpawnParams.xform.scale *= carrierShip->getTransform().scale;
			//}
			particleSpawnParams.xform.scale = glm::vec3(0.25f);
			return particleSpawnParams;
		};
		auto sharedExplosionLogic = [this, makeDefaultExplosion](ParticleSystem& particleSys) 
		{
			if (sfx_explosionEmitter)
			{
				sfx_explosionEmitter->stop();
				sfx_explosionEmitter->setPosition(getWorldPosition());
				sfx_explosionEmitter->play();
			}

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

	glm::vec3 ShipPlacementEntity::getSpawnUp_wn()
	{
		using namespace glm;
		if (!cache_spawnUp_wn.has_value())
		{
			cache_spawnUp_wn = normalize(vec3(getParentXform() * getSpawnXform() *  vec4(up_ln, 0)));
		}
		return *cache_spawnUp_wn;
	}

	glm::vec3 ShipPlacementEntity::getSpawnRight_wn()
	{
		using namespace glm;
		if (!cache_spawnRight_wn.has_value())
		{
			cache_spawnRight_wn = normalize(vec3(getParentXform() * getSpawnXform() * vec4(right_ln, 0)));
		}
		return *cache_spawnRight_wn;
	}

	glm::vec3 ShipPlacementEntity::getSpawnForward_wn()
	{
		using namespace glm;
		if (!cache_spawnForward_wn.has_value())
		{
			cache_spawnForward_wn = normalize(vec3(getParentXform() * getSpawnXform() * vec4(forward_ln, 0)));
		}
		return *cache_spawnForward_wn;
	}

	void ShipPlacementEntity::updateModelMatrixCache()
	{
		//perhaps clear cache function that different functions can call to whip out all cache
		cachedWorldPosition = std::nullopt;
		cachedWorldForward_n = std::nullopt;
		cachedWorldUp_n = std::nullopt;
		cache_spawnUp_wn = std::nullopt;
		cache_spawnRight_wn = std::nullopt;
		cache_spawnForward_wn = std::nullopt;

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

	void ShipPlacementEntity::onNewOwnerSet(const sp<Ship>& owner)
	{
		if (const sp<const SpawnConfig>& spawnConfig = owner->getSpawnConfig())
		{
			sfx_explosionConfig = spawnConfig->getConfig_sfx_explosion();

			if (sfx_explosionConfig.assetPath.size() != 0)
			{
				if (!sfx_explosionEmitter)
				{
					sfx_explosionEmitter = GameBase::get().getAudioSystem().createEmitter();
				}
				sfx_explosionConfig.configureEmitter(sfx_explosionEmitter);
				sfx_explosionEmitter->setPriority(AudioEmitterPriority::GAMEPLAY_PLAYER);
			}
			else
			{
				sfx_explosionEmitter = nullptr;
			}
		}
		else
		{
			sfx_explosionConfig = SoundEffectSubConfig{};
		}
	}

	void ShipPlacementEntity::adjustHP(float amount, const sp<WorldEntity>& optionalInstigator /*= nullptr*/)
	{
		if (HitPointComponent* hpComp = getGameComponent<HitPointComponent>())
		{
			const HitPoints& hp = hpComp->getHP();

			//#TODO: note: this modifier needs to be done in the hp component as it will not be applied if accessed through different means
			//hpComp->adjust(bHasGeneratorPower ? glm::min(amount / 2, 1) : amount);//half the damage if generators are running
			hpComp->adjust(amount);

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

						//if player, have carrier send a ship after player
						if (OwningPlayerComponent* owningPlayerComp = optionalInstigator ? optionalInstigator->getGameComponent<OwningPlayerComponent>() : nullptr)
						{
							if (owningPlayerComp->hasOwningPlayer() && weakOwner)
							{
								weakOwner->requestSpawnFighterAgainst(optionalInstigator);
							}
						}
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

	void ShipPlacementEntity::setHasGeneratorPower(bool bValue)
	{
		bHasGeneratorPower = bValue;

		if (HitPointComponent* hpComp = getGameComponent<HitPointComponent>())
		{
			hpComp->setDamageReductionFactor(bHasGeneratorPower ? 0.5f : 1.f);
		}
	}

	void ShipPlacementEntity::setTarget(const wp<TargetType>& newTarget)
	{
		myTarget = newTarget;
		onTargetSet(myTarget ? myTarget.get() : nullptr);
	}

	void ShipPlacementEntity::setForwardLocalSpace(glm::vec3 newForward_ls)
	{
		forward_ln = glm::normalize(newForward_ls);
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

	void ShipPlacementEntity::setWeakOwner(const sp<Ship>& owner)
	{
		weakOwner = owner;

		onNewOwnerSet(owner);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// turret entity
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void TurretPlacement::tick(float dt_sec)
	{
		using namespace glm;
		
		Parent::tick(dt_sec);

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
			const float distToTarget = glm::length(toTarget);
			const vec3 toTarget_n = toTarget / distToTarget;
			const vec3 myForward_wn = getWorldForward_n();
			const vec3 stationaryForward_n = getWorldStationaryForward_n();

			const Transform& xform = getTransform();
			Transform newXform = xform;

			targetRequest.timeWithoutTargetSec = 0.f;

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

					//frame basis vectors make orthonormal basis, so we can project to that plane as if it were an x/y coordinate system
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
				spawnData.sfx = projectileSFX;
				spawnData.owner = sp_this();
				PointLight_Deferred::UserData pointLightData;
				pointLightData.diffuseIntensity = teamData.color;
				spawnData.projectileLightData = pointLightData;

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

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// attempt to drop target if it has gotten too far away
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (distToTarget > targetingData.dropTargetDistance)
			{
				setTarget(sp<TargetType>(nullptr));
			}
		}
		else /*noTarget*/
		{
			targetRequest.timeWithoutTargetSec += dt_sec;
			bool bIsServer = true;
			if (!targetRequest.bDispatchedTargetRequest && targetRequest.timeWithoutTargetSec > targetRequest.waitBeforeRequestSec && bIsServer)
			{ 
				targetRequest.bDispatchedTargetRequest = true;

				if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
				{
					if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
					{
						if (ServerGameMode_SpaceBase* gm = spaceLevel->getServerGameMode_SpaceBase())
						{
							gm->addTurretNeedingTarget(sp_this());
						}
					}
				}
			}
		}
		if constexpr (bCOMPILE_DEBUG_TURRET)
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

	void TurretPlacement::updateModelMatrixCache()
	{
		cache_worldStationaryForward_n = std::nullopt;
		Parent::updateModelMatrixCache();
	}

	void TurretPlacement::onTargetSet(TargetType* rawTarget)
	{
		Parent::onTargetSet(rawTarget);

		targetRequest.bDispatchedTargetRequest = false; //we don't know if we've been given target from gamemode or not, regardless gamemdoe will clean up. so clear this.
		targetRequest.timeWithoutTargetSec = 0.f; //regardless of if rawTarget is null, reset timer to give logic clean state
	}

	void TurretPlacement::notifyDamagingHit(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{
		bool bShouldSwitchTarget = false; 

		if (myTarget && hitProjectile.owner && myTarget.fastGet() != hitProjectile.owner.get())
		{
			glm::vec3 attackerPosition = hitProjectile.owner->getWorldPosition();
			glm::vec3 targetPosition = myTarget->getWorldPosition();
			glm::vec3 myPos = getWorldPosition();


			float switchDistPerc = 0.75; //if it is % closer than current target, switch.
			float targetDist = glm::length(targetPosition - myPos);
			float attackerDist = glm::length(attackerPosition - myPos);
			bShouldSwitchTarget |= (targetDist * switchDistPerc) > attackerDist;

			//perhaps also consider if target is nearly dead?
			//bShouldSwitchTarget &= !NearlyDead;

		}
		else if(!myTarget)
		{
			//if we don't have a target, try to switch
			bShouldSwitchTarget = true;
		}

		if (bShouldSwitchTarget && hitProjectile.owner)
		{
			wp<TargetType> ownerAsTargetType = hitProjectile.owner->requestTypedReference_Safe<TargetType>();
			if (!ownerAsTargetType.expired())
			{
				setTarget(ownerAsTargetType);
			}
		}
	}

	void TurretPlacement::onNewOwnerSet(const sp<Ship>& owner)
	{
		ShipPlacementEntity::onNewOwnerSet(owner);

		if (owner)
		{
			const sp<const SpawnConfig>& shipConfig = owner->getSpawnConfig();
			if (shipConfig)
			{
				projectileSFX = shipConfig->getConfig_sfx_projectileLoop(); //probably will end up needing a unique sound per ship
			}
		}
		else
		{
			projectileSFX = SoundEffectSubConfig{};
		}
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

		sp<RNG> placementRNG = GameBase::get().getRNGSystem().getNamedRNG("placement");
		targetRequest.waitBeforeRequestSec += placementRNG->getFloat<float>(0, variabilityInTargetWaitSec);


	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Communications placement
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static char const* const activeSeekerShader_forward_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 modelUV;  

		uniform mat4 projection_view;
		uniform mat4 model;
		uniform vec3 uniformColor;

		out vec2 uv;
		out vec3 color;

		void main(){
			gl_Position = projection_view * model * vec4(position, 1.0f);
			color = uniformColor;
			uv = modelUV;
		}
	)";
	static char const* const activeSeekerShader_forward_fs = R"(
		#version 330 core
		out vec4 fragColor;

		in vec3 color;
		in vec2 uv;

		uniform vec3 uniformColor;
		uniform sampler2D tessellateTex;

		void main()
		{
			////////////////////////////////////////////
			//This is all taken from shield effect, consult that shader for questions
			////////////////////////////////////////////
			float fractionComplete = 0.5f; //hard code time dependent values

			////////////////////////////////////////////
			//get a grayscale tessellated pattern

			vec2 preventAlignmentOffset = vec2(0.1, 0.2);
			float medMove = 0.5f *fractionComplete;
			float smallMove = 0.5f *fractionComplete;

			vec2 baseEffectUV = uv * vec2(5,5);
			vec2 mediumTessUV_UR = uv * vec2(10,10) + vec2(medMove, medMove) + preventAlignmentOffset;
			vec2 mediumTessUV_UL = uv * vec2(10,10) + vec2(medMove, -medMove);

			vec2 smallTessUV_UR = uv * vec2(20,20) + vec2(smallMove, smallMove) + preventAlignmentOffset;
			vec2 smallTessUV_UL = uv * vec2(20,20) + vec2(smallMove, -smallMove);

			//invert textures so black is now white and white is black (1-color); 
			vec4 small_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
			vec4 small_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
			vec4 med_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UR);
			vec4 med_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UL);

			vec4 grayScale = 0.25f*small_ur + 0.25f*small_ul + 0.25f*med_ur + 0.25f*med_ur;

			////////////////////////////////////////////
			// filter out some color
			////////////////////////////////////////////
			float cutoff = max(fractionComplete, 0.5f); //starts to disappear once fractionComplete takes over
			if(grayScale.r < cutoff)
			{
				discard;
			}
				
			////////////////////////////////////////////
			// colorize grayscale
			////////////////////////////////////////////
			vec3 texColor = grayScale.rgb * uniformColor;

			 //texColor = texture(tessellateTex, uv).xyz;

		#define TONE_MAP_HDR 0
		#if TONE_MAP_HDR
			vec3 reinHardToneMapped = (texColor) / (1 + texColor);
			fragColor = vec4(reinHardToneMapped, 1.0f);
		#else
			fragColor = vec4(texColor, 1.0f);
		#endif
		}
	)";

	static char const* const activeSeekerShader_deferred_fs = R"(
		#version 330 core

		//framebuffer locations 
		layout (location = 0) out vec3 position;
		layout (location = 1) out vec3 normal;
		layout (location = 2) out vec4 albedo_spec;

		in vec3 color;
		in vec2 uv;

		uniform vec3 uniformColor;
		uniform sampler2D tessellateTex;

		void main()
		{
			vec4 fragColor;

			////////////////////////////////////////////
			//This is all taken from shield effect, consult that shader for questions
			////////////////////////////////////////////
			float fractionComplete = 0.5f; //hard code time dependent values

			////////////////////////////////////////////
			//get a grayscale tessellated pattern

			vec2 preventAlignmentOffset = vec2(0.1, 0.2);
			float medMove = 0.5f *fractionComplete;
			float smallMove = 0.5f *fractionComplete;

			vec2 baseEffectUV = uv * vec2(5,5);
			vec2 mediumTessUV_UR = uv * vec2(10,10) + vec2(medMove, medMove) + preventAlignmentOffset;
			vec2 mediumTessUV_UL = uv * vec2(10,10) + vec2(medMove, -medMove);

			vec2 smallTessUV_UR = uv * vec2(20,20) + vec2(smallMove, smallMove) + preventAlignmentOffset;
			vec2 smallTessUV_UL = uv * vec2(20,20) + vec2(smallMove, -smallMove);

			//invert textures so black is now white and white is black (1-color); 
			vec4 small_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
			vec4 small_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
			vec4 med_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UR);
			vec4 med_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UL);

			vec4 grayScale = 0.25f*small_ur + 0.25f*small_ul + 0.25f*med_ur + 0.25f*med_ur;

			////////////////////////////////////////////
			// filter out some color
			////////////////////////////////////////////
			float cutoff = max(fractionComplete, 0.5f); //starts to disappear once fractionComplete takes over
			if(grayScale.r < cutoff)
			{
				discard;
			}
				
			////////////////////////////////////////////
			// colorize grayscale
			////////////////////////////////////////////
			vec3 texColor = grayScale.rgb * uniformColor;
			//vec3 texColor = texture(tessellateTex, uv).xyz;

			fragColor = vec4(texColor, 1.0f);
			
			position = position;
			albedo_spec.rgb = fragColor.rgb;
			albedo_spec.a = 0.f;
			normal = vec3(0.f); //this is an emissive object, don't render and lighting on it
		}
	)";


	void CommunicationPlacement::tick(float dt_sec)
	{
		static DebugRenderSystem& debugRenderer = GameBase::get().getDebugRenderSystem();
		using namespace glm;

		if (hasStartedDestructionPhase() || isPendingDestroy())
		{
			return;
		}

		timeSinceFire_sec += dt_sec;

		Parent::tick(dt_sec);
		if (myTarget)
		{
			targetRequest.timeWithoutTargetSec = 0.f;

			if (TargetType* target = myTarget.fastGet())
			{
				const vec3 targetPos_wp = target->getWorldPosition();
				const vec3 myWorldPos_wp = getWorldPosition();
				const vec3 toTarget_wv = targetPos_wp - myWorldPos_wp;
				const float distToTarget = glm::length(toTarget_wv);
				const vec3 toTarget_wn = toTarget_wv / distToTarget;
				const vec3 forward_wn = getWorldForward_n();

				const vec3 spawnRight_wn = getSpawnRight_wn();
				const vec3 spawnForward_wn = getSpawnForward_wn();
				const vec3 spawnUp_wn = getSpawnUp_wn();
				const float projectToTargetOnUp = glm::dot(spawnUp_wn, toTarget_wn);

				//note: projecting to plane must be done on an orthonormal plane for it to work like expected; that is the case for this.
				const vec3 targetProj_wn = normalize(Utils::project(toTarget_wn, spawnRight_wn) + Utils::project(toTarget_wn, spawnForward_wn));
				const vec3 myForwardProj_wn = normalize(Utils::project(forward_wn, spawnRight_wn) + Utils::project(forward_wn, spawnForward_wn));

				if (!Utils::vectorsAreSame(targetProj_wn, myForwardProj_wn))
				{
					bool bRotationMatchesUp = dot(glm::cross(myForwardProj_wn, targetProj_wn), spawnUp_wn) > 0.f;
					float angleBetweenTargetAndMyForward_rad = Utils::getRadianAngleBetween(targetProj_wn, myForwardProj_wn);

					constexpr float minRotation = glm::radians(1.0f);
					if (angleBetweenTargetAndMyForward_rad > minRotation)
					{
						const Transform& xform = getTransform();
						Transform newXform = xform;
#define FIX_SATELLITE_UP_VECTOR 0
#if FIX_SATELLITE_UP_VECTOR 
						//due to numerical imprecision, the satellite may drift over time due to piling up rotations, so we probably want to correct its up vector.
						//using last frame data to avoid doing extra transformations; drift over frames should be small so this should be okay
						vec3 myPreviousUp_wn = normalize(vec3(getParentXLocalModelMatrix() * vec4(up_ln, 0)));
						if (dot(myPreviousUp_wn, spawnUp_wn) < 0.999f)
						{
							float angleToCorrectUp_rad = Utils::getRadianAngleBetween(myPreviousUp_wn, spawnUp_wn);
							vec3 rotAxis = normalize(glm::cross(myPreviousUp_wn, spawnUp_wn));
							quat upCorrectRot = glm::angleAxis(angleToCorrectUp_rad, rotAxis);
							newXform.rotQuat = upCorrectRot * newXform.rotQuat;
						}
#endif //FIX_SATELLITE_UP_VECTOR 

						float rotThisTick_rad = glm::min(angleBetweenTargetAndMyForward_rad, rotationSpeed_radsec * dt_sec); //if small angle, just use that as rotation
						
						//rotation needs to be done in local space, however the angle is the same in both spaces.
						//quat turnRot = glm::angleAxis(bRotationMatchesUp ? rotThisTick_rad : -rotThisTick_rad, spawnUp_wn);
						quat turnRot = glm::angleAxis(bRotationMatchesUp ? rotThisTick_rad : -rotThisTick_rad, normalize(vec3(getSpawnXform() * vec4(getLocalUp_n(),0)))); 
						
						newXform.rotQuat = turnRot * newXform.rotQuat;
						setTransform(newXform);
					}
				}

				bool bTargetIsAboveHorizon = projectToTargetOnUp > 0.f;
				if (timeSinceFire_sec > fireCooldown_sec && !activeSeeker.has_value() && bTargetIsAboveHorizon)
				{
					activeSeeker = HealSeeker{};
					activeSeeker->xform.position = getParentXLocalModelMatrix() * vec4(barrelLocation_lp, 1.f);
					activeSeeker->xform.scale = vec3(0.5f);
					timeSinceFire_sec = 0.f;
				}

				if(activeSeeker)
				{
					vec3 toTarget_wv = targetPos_wp - (activeSeeker->xform.position);
					vec3 toTarget_wn = glm::normalize(toTarget_wv);
					vec3 deltaPos = toTarget_wn * activeSeeker->speed * dt_sec;
					deltaPos = glm::length2(deltaPos) < glm::length2(toTarget_wv) ? deltaPos : toTarget_wn; //if close to target, move right to it.
					activeSeeker->xform.position += deltaPos;
					if (glm::length2(toTarget_wv) < glm::length2(activeSeeker->completeRadius))
					{
						if (HitPointComponent* hpComp = target->getGameComponent<HitPointComponent>())
						{
							hpComp->adjust(25.f); //heal!
						}
						else
						{
							log(__FUNCTION__, LogLevel::LOG_WARNING, "attempted to heal target without any hp component");
						}
						activeSeeker = std::nullopt;
					}
				}

				if constexpr (bCOMPILE_DEBUG_SATELLITE)
				{
					debugRenderer.renderLine(myWorldPos_wp, myWorldPos_wp + 10.f*spawnRight_wn, vec3(1, 0, 0));
					debugRenderer.renderLine(myWorldPos_wp, myWorldPos_wp + 10.f*spawnUp_wn, vec3(0, 1, 0));
					debugRenderer.renderLine(myWorldPos_wp, myWorldPos_wp + 10.f*spawnForward_wn, vec3(0, 0, 1));
					debugRenderer.renderLine(myWorldPos_wp, myWorldPos_wp + 10.f*myForwardProj_wn, color::metalicgold());
					debugRenderer.renderLine(myWorldPos_wp, myWorldPos_wp + 10.f*targetProj_wn, color::lightPurple());
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// attempt to drop target if it has gotten too far away
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (distToTarget > targetingData.dropTargetDistance)
				{
					setTarget(sp<TargetType>(nullptr));
				}
			}

		}
		else /*noTarget*/
		{
			targetRequest.timeWithoutTargetSec += dt_sec;
			bool bIsServer = true;
			if (!targetRequest.bDispatchedTargetRequest && targetRequest.timeWithoutTargetSec > targetRequest.waitBeforeRequestSec && bIsServer)
			{
				targetRequest.bDispatchedTargetRequest = true;

				if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
				{
					if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
					{
						if (ServerGameMode_SpaceBase* gm = spaceLevel->getServerGameMode_SpaceBase())
						{
							gm->addHealerNeedingTarget(sp_this());
						}
					}
				}
			}
		}
		if constexpr (bCOMPILE_DEBUG_SATELLITE)
		{
			mat4 localBox_m(1.f);
			localBox_m = glm::translate(mat4(1.f), barrelLocation_lp);
			localBox_m = glm::scale(localBox_m, vec3(0.1f));
			debugRenderer.renderCube(getParentXLocalModelMatrix() * localBox_m, glm::vec3(1, 0, 0));
		}
	}

	void CommunicationPlacement::postConstruct()
	{
		Parent::postConstruct();

		//just using the planet model as it is a sphere and the particle system (shield effect) accepts models
		if (!seekerModel)
		{
			seekerModel = GameBase::get().getAssetSystem().loadModel("GameData/mods/SpaceArcade/Assets/Models3D/Planet/textured_planet.obj");
			seekerShader = GameBase::get().getRenderSystem().usingDeferredRenderer() ? 
				new_sp<Shader>(activeSeekerShader_forward_vs, activeSeekerShader_deferred_fs, false) :
				new_sp<Shader>(activeSeekerShader_forward_vs, activeSeekerShader_forward_fs, false);

			if (GameBase::get().getAssetSystem().loadTexture("GameData/engine_assets/TessellatedShapeRadials.png", tessellatedTextureID))
			{
				//seekerShader->setUniform1i("tessellateTex", 0); dont in tick for code readability and locality; probably should do here for efficiency assuming texture slot doesn't change
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Failed to get tessellated texture!");
			}
		}

		sp<RNG> placementRNG = GameBase::get().getRNGSystem().getNamedRNG("placement");
		targetRequest.waitBeforeRequestSec += placementRNG->getFloat<float>(0, variabilityInTargetWaitSec);
	}

	void CommunicationPlacement::render(Shader& shader)
	{
		Parent::render(shader);

		using namespace glm;
		if (activeSeeker)
		{
			static RenderSystem& renderSystem = GameBase::get().getRenderSystem();
			if (const RenderData* frd = renderSystem.getFrameRenderData_Read(GameBase::get().getFrameNumber()))
			{
				seekerShader->use();
				mat4 model = activeSeeker->xform.getModelMatrix();
				mat4 projection_view = frd->projection * frd->view;

				seekerShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));
				seekerShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				seekerShader->setUniform3f("uniformColor", color::green() * (GameBase::get().getRenderSystem().isUsingHDR() ? 3.f : 1.f)); //@hdr_tweak
				
				const uint32_t textureSlot = GL_TEXTURE0;
				ec(glActiveTexture(textureSlot));
				ec(glBindTexture(GL_TEXTURE_2D, tessellatedTextureID));
				seekerShader->setUniform1i("tessellateTex", textureSlot - GL_TEXTURE0);

				seekerModel->draw(*seekerShader, false);
			}
		}
	}

	void CommunicationPlacement::onTargetSet(TargetType* rawTarget)
	{
		Parent::onTargetSet(rawTarget);
		if (!rawTarget)
		{
			if (activeSeeker)
			{
				activeSeeker = std::nullopt;
			}
		}

		targetRequest.bDispatchedTargetRequest = false; //we don't know if we've been given target from gamemode or not, regardless gamemdoe will clean up. so clear this.
		targetRequest.timeWithoutTargetSec = 0.f; //regardless of if rawTarget is null, reset timer to give logic clean state
	}

	void CommunicationPlacement::replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig)
	{
		Parent::replacePlacementConfig(newConfig, owningConfig);

		const glm::mat4& spawnXform = getSpawnXform();
	}

	void CommunicationPlacement::onDestroyed()
	{
		Parent::onDestroyed();

		if (activeSeeker)  //may not be needed
		{ 
			activeSeeker = std::nullopt; 
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
