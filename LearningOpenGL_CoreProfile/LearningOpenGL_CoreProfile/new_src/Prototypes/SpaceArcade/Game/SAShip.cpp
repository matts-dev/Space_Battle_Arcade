#include "SAShip.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAGameEntity.h"
#include "SAProjectileSystem.h"
#include "../GameFramework/SAAIBrainBase.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../GameFramework/SAParticleSystem.h"
#include "../GameFramework/EngineParticles/BuiltInParticles.h"
#include "AI/SAShipAIBrain.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "../Tools/SAUtilities.h"
#include "../GameFramework/SABehaviorTree.h"
#include "Components/ShipEnergyComponent.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SARandomNumberGenerationSystem.h"
#include "../Tools/color_utils.h"
#include "../GameFramework/SADebugRenderSystem.h"
#include "Cameras/SAShipCamera.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../GameFramework/Components/CollisionComponent.h"
#include "../Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"
#include "../GameFramework/SALevelSystem.h"
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../GameFramework/SAPlayerSystem.h"
#include "../GameFramework/SAPlayerBase.h"
#include "FX/SharedGFX.h"
#include "Cheats/SpaceArcadeCheatManager.h"
#include "SAShipPlacements.h"
#include "SAModSystem.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Runtime flags
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool bDrawAvoidance_debug = false;
	bool bForcePlayerAvoidance_debug = false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// methods
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Ship::Ship(const SpawnData& spawnData)
		: RenderModelEntity(spawnData.spawnConfig->getModel(), spawnData.spawnTransform),
		collisionData(spawnData.spawnConfig->toCollisionInfo()), 
		cachedTeamIdx(spawnData.team)
	{
		overlappingNodes_SH.reserve(10);
		primaryProjectile = spawnData.spawnConfig->getPrimaryProjectileConfig();
		shipData = spawnData.spawnConfig;
		bCollisionReflectForward = shipData->getCollisionReflectForward();

		ctor_configureObjectivePlacements();

		////////////////////////////////////////////////////////
		// Make components
		////////////////////////////////////////////////////////
		createGameComponent<BrainComponent>();
		createGameComponent<TeamComponent>();
		CollisionComponent* collisionComp = createGameComponent<CollisionComponent>();
		energyComp = createGameComponent<ShipEnergyComponent>();
		getGameComponent<TeamComponent>()->setTeam(spawnData.team);

		////////////////////////////////////////////////////////
		// any extra component configuration
		////////////////////////////////////////////////////////
		updateTeamDataCache();
		collisionComp->setCollisionData(collisionData);
		collisionComp->setKinematicCollision(spawnData.spawnConfig->requestCollisionTests());
	}

	void Ship::postConstruct()
	{
		rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

		for (const AvoidanceSphereSubConfig& sphereConfig : shipData->getAvoidanceSpheres())
		{
			//must wait for postConstruct because we need to pass sp_this() for owner field.
			sp<AvoidanceSphere> avoidanceSphere = new_sp<AvoidanceSphere>(sphereConfig.radius, sphereConfig.localPosition, sp_this());
			avoidanceSpheres.push_back(avoidanceSphere);
		}

		//WARNING: caching world sp will create cyclic reference
		if (LevelBase* world = getWorld())
		{
			//bug? not using below. need to update collisionData transform?
			//Transform xform = getTransform();
			//glm::mat4 xform_m = xform.getModelMatrix();
			collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB());
		}
		else
		{
			throw std::logic_error("World entity being created but there is no containing world");
		}

		if (TeamComponent* team = getGameComponent<TeamComponent>())
		{
			team->onTeamChanged.addWeakObj(sp_this(), &Ship::handleTeamChanged);
			updateTeamDataCache();
		}

		for (const sp<ShipPlacementEntity>& placement : communicationEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }
		for (const sp<ShipPlacementEntity>& placement : defenseEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }
		for (const sp<ShipPlacementEntity>& placement : turretEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }
		

#if COMPILE_CHEATS
		SpaceArcadeCheatSystem& cheatSystem = static_cast<SpaceArcadeCheatSystem&>(GameBase::get().getCheatSystem());
		cheatSystem.oneShotShipObjectivesCheat.addWeakObj(sp_this(), &Ship::cheat_OneShotPlacements);
		cheatSystem.destroyAllShipObjectivesCheat.addWeakObj(sp_this(), &Ship::cheat_DestroyAllShipPlacements);
		if(turretEntities.size() > 0) cheatSystem.turretsTargetPlayerCheat.addWeakObj(sp_this(), &Ship::cheat_TurretsTargetPlayer);
#endif //COMPILE_CHEATS
	}

	Ship::~Ship()
	{
		//#TODO create LP bindings for delegates. Have ships ticked via LP ticker. Draw debug box for LP ticker to quickly spot memory leaks when they happen. #memory_leaks
		//adding for debug 
#define LOG_SHIP_DTOR 0
#if LOG_SHIP_DTOR 
		log("logShip", LogLevel::LOG, "ship destroyed");
#endif 
	}

	glm::vec4 Ship::getForwardDir() const
	{
		//#optimize #todo cache per frame/move update; transforming vector (perhaps gameBase can track a framenumber with an inline call)
		const Transform& transform = getTransform();

		// #Warning this doesn't include any parent transformations #parentxform; those can also be cached per frame
		//Currently there is no system for parent/child scene nodes for these. But if there were/ever is, it should
		//get the parent transform and use that like : parentXform * rotMatrix * pointingDir; but I believe non-unfirom scaling will cause
		//issues with vectors (like normals) and some care (normalMatrix, ie inverse transform), or (no non-uniform scales) may be needed to make sure vectors are correct
		glm::vec4 pointingDir = localForwardDir_n();//#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * pointingDir;

		return rotDir;
	}

	glm::vec4 Ship::getUpDir() const
	{
		//#optimize follow optimizations done in getForwardDir if any are made
		const Transform& transform = getTransform();
		glm::vec4 upDir{ 0, 1, 0, 0 }; //#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * upDir;
		return rotDir;
	}

	glm::vec4 Ship::getRightDir() const
	{
		//#optimize follow optimizations done in getForwardDir if any are made
		const Transform& transform = getTransform();
		glm::vec4 upDir{ 1, 0, 0, 0 }; //#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * upDir;
		return rotDir;
	}

	glm::vec4 Ship::rotateLocalVec(const glm::vec4& localVec)
	{
		const Transform& transform = getTransform();
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * localVec;
		return rotDir;
	}

	float Ship::getMaxTurnAngle_PerSec() const
	{
		//if speeds are slower than 1.0, we increase the max turn radius. eg 1/0.5  = 2;
		return glm::pi<float>() * glm::clamp(1.f / (currentSpeedFactor+0.0001f), 0.f, 3.f);
	}

	void Ship::setVelocityDir(glm::vec3 inVelocity)
	{
		NAN_BREAK(inVelocity);
		velocityDir_n = glm::normalize(inVelocity);
	}

	glm::vec3 Ship::getVelocity(const glm::vec3 customVelocityDir_n)
	{
		//apply a speed increase if you have a full tank of energy (this helps create distance when two ai's are dogfighting)
		constexpr float energyThresholdForBoost = 75.f;
		float highEnergyBoost = glm::clamp(energyComp->getEnergy() / 75.f, 1.0f, 1.1f);

		return customVelocityDir_n
			* currentSpeedFactor * getMaxSpeed() * speedGamifier
			* highEnergyBoost 
			* adjustedBoost;
	}

	glm::vec3 Ship::getVelocity()
	{
		return getVelocity(velocityDir_n);
	}

	void Ship::setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig)
	{
		primaryProjectile = projectileConfig;
	}

	void Ship::updateTeamDataCache()
	{
		//#TODO perhaps just cache temp comp directly?

		if (TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			cachedTeamIdx = teamComp->getTeam();
			const std::vector<TeamData>& teams = shipData->getTeams();

			if (cachedTeamIdx >= teams.size()) { cachedTeamIdx = 0;}
		
			if (teams.size() > 0)
			{
				assert(teams.size() > cachedTeamIdx);
				cachedTeamData = teams[cachedTeamIdx];

				static auto applyTeamToObjectives = [](const std::vector<sp<ShipPlacementEntity>>& objectives, size_t teamIdx) 
				{
					for (const sp<ShipPlacementEntity>& entity : objectives)
					{
						if (TeamComponent* teamComp = entity ? entity->getGameComponent<TeamComponent>() : nullptr)
						{
							teamComp->setTeam(teamIdx);
						}
					}
				};
				applyTeamToObjectives(defenseEntities, cachedTeamIdx);
				applyTeamToObjectives(communicationEntities, cachedTeamIdx);
				applyTeamToObjectives(turretEntities, cachedTeamIdx);
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "NO TEAM DATA ASSIGNED TO SHIP");
			}


		}
	}

	void Ship::handleTeamChanged(size_t oldTeamId, size_t newTeamId)
	{
		updateTeamDataCache();
	}

	void Ship::setRenderAvoidanceSpheres(bool bNewRenderAvoidance)
	{
		bRenderAvoidanceSpheres = bNewRenderAvoidance;
	}

	void Ship::draw(Shader& shader)
	{
		glm::mat4 configuredModelXform = collisionData->getRootXform(); //#TODO #REFACTOR this ultimately comes from the spawn config, it is somewhat strange that we're reading this from collision data.But we need this to render models to scale.
		glm::mat4 rawModel = getTransform().getModelMatrix();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(rawModel * configuredModelXform)); //unfortunately the spacearcade game is setting this uniform, so we're hitting this hot code twice.
		shader.setUniform3f("objectTint", cachedTeamData.teamTint);
		RenderModelEntity::draw(shader);

		if (avoidanceSpheres.size() > 0 && Ship::bRenderAvoidanceSpheres)
		{
			for (sp<AvoidanceSphere>& avoidSphere : avoidanceSpheres)
			{
				avoidSphere->render();
			}
		}

		//assuming same shader for ship will be used for placements
		static const auto& renderPlacements = [](const std::vector<sp<ShipPlacementEntity>>& placements, Shader& shader)
		{
			for (const sp<ShipPlacementEntity>& placement : placements)
			{
				if (placement)
				{
					placement->draw(shader);
				}
			}
		};
		renderPlacements(defenseEntities, shader);
		renderPlacements(communicationEntities, shader);
		renderPlacements(turretEntities, shader);
	}

	void Ship::onDestroyed()
	{
		RenderModelEntity::onDestroyed();
		collisionHandle = nullptr; //release spatial hashing information
	}

	void Ship::onPlayerControlTaken()
	{
		if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
		{
			if (AIBrain* brain = brainComp->getBrain())
			{
				brain->sleep();
			}
		}
		bEnableAvoidanceFields = false || bForcePlayerAvoidance_debug;;
	}

	void Ship::onPlayerControlReleased()
	{
		if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
		{
			if (AIBrain* brain = brainComp->getBrain())
			{
				brain->awaken();
			}
		}
		bEnableAvoidanceFields = true;

		if (shipCamera)  //release ship camera so it doesn't affect ship any longer
		{
			shipCamera->followShip(sp<Ship>(nullptr));
			shipCamera = nullptr;  //also set null in case the old camera is being cached, that way we always generate a new camera.
		} 
	}

	sp<CameraBase> Ship::getCamera()
	{
		if (!shipCamera)
		{
			shipCamera = new_sp<ShipCamera>();

			const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
			shipCamera->registerToWindowCallbacks_v(primaryWindow);
			shipCamera->followShip(sp_this());
		}
		return shipCamera;
	}

	void Ship::fireProjectile(BrainKey privateKey)
	{
		//#optimize: set a default projectile config so this doesn't have to be checked every time a ship fires? reduce branch divergence
		if (primaryProjectile)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();

			ProjectileSystem::SpawnData spawnData;
			//#TODO #scenenodes doesn't account for parent transforms
			spawnData.direction_n = velocityDir_n;
			spawnData.start = spawnData.direction_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.owner = this;

			projectileSys->spawnProjectile(spawnData, *primaryProjectile); 
		}
	}

	void Ship::fireProjectileInDirection(glm::vec3 dir_n) const
	{
		if (primaryProjectile && length2(dir_n) > 0.001 && FIRE_PROJECTILE_ENABLED)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();

			ProjectileSystem::SpawnData spawnData;
			//#TODO #scenenodes doesn't account for parent transforms
			spawnData.direction_n = glm::normalize(dir_n);
			spawnData.start = spawnData.direction_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.owner = this;

			projectileSys->spawnProjectile(spawnData, *primaryProjectile);
		}
	}

	void Ship::moveTowardsPoint(const glm::vec3& moveLoc, float dt_sec, float speedAmplifier, bool bRoll, float turnAmplifier, float viscosity)
	{
		//#TODO #REFACTOR #cleancode #input_vector this should in influencing an input vector, rather than directly influencing the velocity; tick should do the visual updates and velocity changes based on input vector
		using namespace glm;

		const Transform& xform = getTransform();
		vec3 targetDir = moveLoc - xform.position;

		vec3 forwardDir_n = glm::normalize(vec3(getForwardDir()));
		vec3 targetDir_n = glm::normalize(targetDir);
		bool bPointingTowardsMoveLoc = glm::dot(forwardDir_n, targetDir_n) >= 0.999f;

		if (!bPointingTowardsMoveLoc)
		{
			vec3 rotationAxis_n = glm::cross(forwardDir_n, targetDir_n);

			float angleBetween_rad = Utils::getRadianAngleBetween(forwardDir_n, targetDir_n);
			float maxTurn_Rad = getMaxTurnAngle_PerSec() * turnAmplifier * dt_sec;
			float possibleRotThisTick = glm::clamp(maxTurn_Rad / angleBetween_rad, 0.f, 1.f);

			//slow down turn if some viscosity is being applied
			float fluidity = glm::clamp(1.f - viscosity, 0.f, 1.f);
			possibleRotThisTick *= fluidity;

			quat completedRot = Utils::getRotationBetween(forwardDir_n, targetDir_n) * xform.rotQuat;
			quat newRot = glm::slerp(xform.rotQuat, completedRot, possibleRotThisTick);

			//roll ship -- we want the ship's right (or left) vector to match this rotation axis to simulate it pivoting while turning
			vec3 rightVec_n = newRot * vec3(1, 0, 0);
			bool bRollMatchesTurnAxis = glm::dot(rightVec_n, rotationAxis_n) >= 0.99f;

			vec3 newForwardVec_n = glm::normalize(newRot * vec3(localForwardDir_n()));
			if (!bRollMatchesTurnAxis && bRoll)
			{
				float rollAngle_rad = Utils::getRadianAngleBetween(rightVec_n, rotationAxis_n);
				float rollThisTick = glm::clamp(maxTurn_Rad / rollAngle_rad, 0.f, 1.f);
				glm::quat roll = glm::angleAxis(rollThisTick, newForwardVec_n);
				newRot = roll * newRot;
			}

			Transform newXform = xform;
			newXform.rotQuat = newRot;
			setTransform(newXform);
			setVelocityDir(newForwardVec_n);
			speedGamifier = speedAmplifier;
		}
		else
		{
			setVelocityDir(forwardDir_n);
			speedGamifier = speedAmplifier;
		}
	}

	void Ship::roll(float rollspeed_rad, float dt_sec, float clamp_rad /*= 3.14f*/)
	{
		using namespace glm;
		Transform newXform = getTransform();
		vec3 forwardDir_n = glm::normalize(vec3(getForwardDir()));

		float roll_rad = glm::clamp(rollspeed_rad * dt_sec, -clamp_rad, clamp_rad);
		glm::quat roll_q = glm::angleAxis(roll_rad, forwardDir_n);
		newXform.rotQuat = roll_q * newXform.rotQuat;
		setTransform(newXform);
	}

	void Ship::adjustSpeedFraction(float targetSpeedFactor, float dt_sec)
	{
		targetSpeedFactor = glm::clamp<float>(targetSpeedFactor, -0.1f, 1.f);

		float toTarget = targetSpeedFactor - currentSpeedFactor;
		float deltaSpeed = engineSpeedChangeFactor * dt_sec;
		if (glm::abs(deltaSpeed) < glm::abs(toTarget))
		{
			//float sign = toTarget > 0 ? 1.f : -1.f; 
			float sign = glm::sign<float>(toTarget);
			currentSpeedFactor = currentSpeedFactor + sign * deltaSpeed;
		}
		else
		{
			currentSpeedFactor = currentSpeedFactor + toTarget;
		}
	}

	void Ship::setNextFrameBoost(float targetSpeedFactor)
	{
		boostNextFrame = targetSpeedFactor;
	}

	bool Ship::fireProjectileAtShip(const WorldEntity& myTarget, std::optional<float> inFireRadius_cosTheta /*=empty*/, float shootRandomOffsetStrength /*= 1.f*/) const
	{
		using namespace glm;
		static float defaultFireRadius_cosTheta = glm::cos(10 * glm::pi<float>() / 180);

		float fireRadius_cosTheta = inFireRadius_cosTheta.value_or(defaultFireRadius_cosTheta);
		vec3 myPos = getWorldPosition();
		vec3 targetPos = myTarget.getWorldPosition();
		vec3 toTarget_n = glm::normalize(targetPos - myPos);
		vec3 myForward_n = vec3(getForwardDir());

		float toTarget_cosTheta = glm::dot(toTarget_n, myForward_n);
		if (toTarget_cosTheta >= fireRadius_cosTheta)
		{
			//correct for velocity
			//#TODO, perhaps get a kinematic component or something to get an idea of the velocity
			vec3 correctTargetPos = targetPos;
			vec3 toFirePos_un = correctTargetPos - myPos;

			//construct basis around target direction
			vec3 fireRight_n = glm::normalize(glm::cross(toFirePos_un, vec3(getUpDir())));
			vec3 fireUp_n = glm::normalize(glm::cross(fireRight_n, toFirePos_un));

			//add some randomness to fire direction, more random of easier AI; randomness also helps correct for twitch movements done by target
			//we could apply this directly the fire direction we pass to ship, but this has a more intuitive meaning and will probably perform better for getting actual hits
			correctTargetPos += fireRight_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;
			correctTargetPos += fireUp_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;

			//fire in direction
			vec3 toFirePos_n = glm::normalize(correctTargetPos - myPos);
			fireProjectileInDirection(toFirePos_n);

			return true;
		}

		return false;
	}

	void Ship::tick(float dt_sec)
	{
		using namespace glm;

		energyComp->notify_tick(dt_sec);

		////////////////////////////////////////////////////////
		// handle boost
		////////////////////////////////////////////////////////
		float boostForFrame = boostNextFrame.value_or(targetBoost);
		boostNextFrame.reset();
		float requestedBoost = boostForFrame - 1.0f; //#TODO this needs adjustment so we don't spend more energy than we actually can use in acceleration
		float requestedBoostCost_sec = ENERGY_BOOST_RATIO_SEC * requestedBoost * dt_sec;
		float boostDelta = 0.f;
		if (energyComp->getEnergy() > requestedBoostCost_sec) //#optimize avoid branch here
		{
			energyComp->spendEnergy(requestedBoostCost_sec);
			boostDelta = boostForFrame - adjustedBoost;
		}
		else
		{
			boostDelta = 1.f - adjustedBoost; //think of this as a 1d vector
		}
		boostDelta = glm::clamp<float>(boostDelta, -BOOST_DECREASE_PER_SEC, BOOST_RAMPUP_PER_SEC);
		adjustedBoost += boostDelta * dt_sec;

		////////////////////////////////////////////////////////
		// handle kinematics
		////////////////////////////////////////////////////////
		tickKinematic(dt_sec);

		Transform xform = getTransform();
		if (!activeShieldEffect.expired())
		{
			//locking wp may be slow as it requires atomic reference count increment; may need to use soft-ptrs if I make that system
			sp<ActiveParticleGroup> activeShield_sp = activeShieldEffect.lock();
			activeShield_sp->xform.rotQuat = xform.rotQuat;
			activeShield_sp->xform.position = xform.position;
		}

		glm::mat4 modelMatrix = xform.getModelMatrix();
		if (avoidanceSpheres.size() > 0)
		{
			for (sp<AvoidanceSphere>& myAvoidSphere : avoidanceSpheres)
			{
				myAvoidSphere->setParentXform(modelMatrix);
			}
		}

		//////////////////////////////////////////////////////////
		// placements - must be handled after updates to position
		//////////////////////////////////////////////////////////
		//#todo #scenenodes no need to pass parent xforms if scene node hierarchy is used
		if (defenseEntities.size() || communicationEntities.size() || turretEntities.size())
		{
			glm::mat4 configXform = collisionData->getRootXform();
			glm::mat4 fullParentXform = modelMatrix * configXform;

			static const auto& tickPlacements = [](float dt_sec, const std::vector<sp<ShipPlacementEntity>>& placements, const glm::mat4& placementParentXform)
			{
				for (const sp<ShipPlacementEntity>& placement : placements) 
				{
					if (placement) 
					{
						placement->setParentXform(placementParentXform);
						placement->tick(dt_sec);
					} 
				}
			};
			tickPlacements(dt_sec, defenseEntities, fullParentXform);
			tickPlacements(dt_sec, communicationEntities, fullParentXform);
			tickPlacements(dt_sec, turretEntities, fullParentXform);
		}

	} 

	void Ship::tickKinematic(float dt_sec)
	{
		using namespace glm;

		bool bAnyCollision = false;
		glm::vec4 lastMTV = glm::vec4(0.f);

		std::optional<glm::vec3> avoidanceVelDir_n = updateAvoidance(dt_sec);

		Transform xform = getTransform();

		xform.position += getVelocity(avoidanceVelDir_n.value_or(velocityDir_n)) * dt_sec; //allow an avoidance corrected velocity dir to be used if one exists
		glm::mat4 movedXform_m = xform.getModelMatrix();

		NAN_BREAK(xform.position);

		//update collision data
		collisionData->updateToNewWorldTransform(movedXform_m);

		LevelBase* world = getWorld();
		if (world && collisionHandle)
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
			worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());

			//test against all overlapping grid nodes
			overlappingNodes_SH.clear();
			worldGrid.lookupNodesInCells(*collisionHandle, overlappingNodes_SH);
			for (sp <SH::GridNode<WorldEntity>> node : overlappingNodes_SH)
			{
				CollisionComponent* otherCollisionComp = node->element.getGameComponent<CollisionComponent>();
				if (otherCollisionComp && otherCollisionComp->requestsCollisionChecks())
				{
					if (CollisionData* otherCollisionData = otherCollisionComp->getCollisionData()) //#todo #optimize so component will always have this data, but it can contain no shapes, to avoid branches. similar to nullobject pattern
					{
						using ShapeData = CollisionData::ShapeData;
						const std::vector<ShapeData>& myShapeData = collisionData->getShapeData();
						const std::vector<ShapeData>& otherShapeData = otherCollisionData->getShapeData();

						//make sure OOB's collide as an optimization
						if (SAT::Shape::CollisionTest(*collisionData->getOBBShape(), *otherCollisionData->getOBBShape()))
						{
							bool bCollision = false;
							size_t numAttempts = 3;
							size_t attempt = 0;
							do
							{
								attempt++;
								bCollision = false;

								glm::vec4 largestMTV = glm::vec4(0.f);
								float largestMTV_len2 = 0.f;
								for (const ShapeData& myShape : myShapeData)
								{
									for (const ShapeData& worldShape : otherShapeData)
									{
										assert(myShape.shape && worldShape.shape);
										glm::vec4 mtv;
										if (SAT::Shape::CollisionTest(*myShape.shape, *worldShape.shape, mtv))
										{
											float mtv_len2 = glm::length2(mtv);
											if (mtv_len2 > largestMTV_len2)
											{
												largestMTV_len2 = mtv_len2;
												largestMTV = lastMTV = mtv;
												bCollision = bAnyCollision = true;
											}
										}
									}
								}
								if (bCollision)
								{
									xform.position += glm::vec3(largestMTV);
									collisionData->updateToNewWorldTransform(xform.getModelMatrix());
								}
							} while (bCollision && attempt < numAttempts);
							//perhaps we should revert back to original xform is it fails collision tests for all attempts.
						}
					}
				}
			}
#if SA_CAPTURE_SPATIAL_HASH_CELLS
			SpatialHashCellDebugVisualizer::appendCells(worldGrid, *collisionHandle);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS

			if (bAnyCollision)
			{
				if (bCollisionReflectForward)
				{
					//MTV in the case of faces will be something like the face normal; however this may not look to great for deflecting off of edges
					glm::vec4 forward_n = getForwardDir();
					glm::vec4 reflectedForward_n = normalize(glm::reflect(forward_n, glm::normalize(lastMTV))); //may not need to normalize
					setVelocityDir(reflectedForward_n);
					xform.rotQuat = Utils::getRotationBetween(forward_n, reflectedForward_n) * xform.rotQuat;
				}
				doShieldFX();
			}

			setTransform(xform); //set after collision is handled so we do not continually update/broadcast events

			if (bAnyCollision && onCollided.numBound() > 0) { onCollided.broadcast(); } //broadcasting after we've updated transform

#define EXTRA_SHIP_DEBUG_INFO 0
#if EXTRA_SHIP_DEBUG_INFO
			{
				DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
				debug.renderCube(xform.getModelMatrix(), color::brightGreen());
			}
#endif
		}
	}

	std::optional<glm::vec3> Ship::updateAvoidance(float dt_sec)
	{
		//#TODO #REFACTOR #cleancode #input_vector this should in influencing an input vector, rather than directly influencing the velocity; tick should do the visual updates and velocity changes based on input vector
		using namespace glm;
		
		std::optional<glm::vec3> newVelocity_n = std::nullopt;
		if (getAvoidanceDampenedVelocity(newVelocity_n))
		{
			const Transform& xform_c = getTransform();
			{ //manual update of ship direction without use of slerp
				glm::vec3 forwardDir_n = vec3(getForwardDir());
				vec3 rotationAxis_n = glm::cross(forwardDir_n, *newVelocity_n);

				float angleBetween_rad = Utils::getRadianAngleBetween(forwardDir_n, *newVelocity_n);
				quat newRot = Utils::getRotationBetween(forwardDir_n, *newVelocity_n) * xform_c.rotQuat;

				vec3 rightVec_n = newRot * vec3(1, 0, 0);
				bool bRollMatchesTurnAxis = glm::dot(rightVec_n, rotationAxis_n) >= 0.99f;
				vec3 newForwardVec_n = glm::normalize(newRot* vec3(localForwardDir_n()));

				Transform newXform = xform_c;
				newXform.rotQuat = newRot;
				setTransform(newXform);
			}
		}
		return newVelocity_n;
	}

	void Ship::notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{
		if (cachedTeamIdx != hitProjectile.team)
		{
			//only damage shield if all objective placements have been destroyed
			if (activePlacements == 0)
			{
				hp.current -= hitProjectile.damage;
				if (hp.current <= 0)
				{
					if(!isPendingDestroy())
					{
						ParticleSystem::SpawnParams particleSpawnParams;
						particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
						particleSpawnParams.xform.position = this->getTransform().position;
						particleSpawnParams.velocity = getVelocity();

						GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);

						if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
						{
							brainComp->setNewBrain(sp<AIBrain>(nullptr));
						}
					}
					destroy(); //perhaps enter a destroyed state with timer to remove actually destroy -- rather than immediately despawning
				}
				else
				{
					doShieldFX();
				}
			}
			else
			{
				//do some FX that shows ship is taking no damage? maybe reflect projectile?
			}
		}
	}

	void Ship::doShieldFX()
	{
		if (!activeShieldEffect.expired())
		{
			sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
			shieldEffect_sp->resetTimeAlive();
		}
		else
		{
			ParticleSystem::SpawnParams particleSpawnParams;
			particleSpawnParams.particle = SharedGFX::get().shieldEffects_ModelToFX->getEffect(getMyModel(), cachedTeamData.shieldColor);
			const Transform& shipXform = this->getTransform();
			particleSpawnParams.xform.position = shipXform.position;
			particleSpawnParams.xform.rotQuat = shipXform.rotQuat;
			particleSpawnParams.xform.scale = shipXform.scale;

			//#TODO #REFACTOR hacky as only considering scale. particle perhaps should use matrices to avoid this, or have list of transform to apply.
			//making the large ships show correct effect. Perhaps not even necessary.
			Transform modelXform = shipData->getModelXform();
			particleSpawnParams.xform.scale *= modelXform.scale;

			activeShieldEffect = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
		}
	}

	bool Ship::getAvoidanceDampenedVelocity(std::optional<glm::vec3>& adjustVel_n) const
	{
		using namespace glm;
		adjustVel_n = std::nullopt;
		float accumulatedStrength = 0.f;

		bool bNoVelocity = Utils::float_equals(glm::length2(velocityDir_n), 0.f);

		#define COMPILE_AVOIDANCE_VECTORS 1
		#if COMPILE_AVOIDANCE_VECTORS
		if (bEnableAvoidanceFields && collisionData && !bNoVelocity)
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				if (SH::SpatialHashGrid<AvoidanceSphere>* avoidGrid = currentLevel->getTypedGrid<AvoidanceSphere>())
				{
					static std::vector<sp<const SH::HashCell<AvoidanceSphere>>> nearbyCells;
					static const int oneTimeInit = [](decltype(nearbyCells)& initVector) { initVector.reserve(10); return 0; } (nearbyCells);

					const Transform& myXform = getTransform();

					static std::vector<AvoidanceSphere*> uniqueNodes;
					static int oneTimieInit = [](decltype(uniqueNodes)& container) {container.reserve(10); return 0; }(uniqueNodes);
					uniqueNodes.clear();

					avoidGrid->lookupCellsForOOB(collisionData->getWorldOBB(), nearbyCells);
					for (const sp<const SH::HashCell<AvoidanceSphere>>& cell : nearbyCells)
					{
						for (const sp<SH::GridNode<AvoidanceSphere>>& node : cell->nodeBucket)
						{
							AvoidanceSphere& avoid = node->element;
							if (avoid.getOwner().fastGet() != this)
							{
								if (std::find(uniqueNodes.begin(), uniqueNodes.end(), &avoid) == uniqueNodes.end()) //must filter as same sphere can be in two cells
								{
									//#TODO this should probably exist for the spatial hash itself.
									uniqueNodes.push_back(&avoid); 
								}
							}
						}
					}

					adjustVel_n = velocityDir_n;

					for (AvoidanceSphere* avoid : uniqueNodes)
					{
						//#TODO this whole logic can probably be inverted to be clear(being based on vectors towards center), but avoiding changing atm as this is known to work
						const vec3 toMe_v = myXform.position - avoid->getWorldPosition();
						float toMeLen = glm::length(toMe_v);
						float radiusFrac = toMeLen / avoid->getRadius(); //This needs clamping [0,1] after processing. should never be zero, avoidance sphere asserts if set to zero radius. 

						radiusFrac = glm::clamp(radiusFrac, 0.f, 1.f);
						float maxAvoidAtRadiusFrac = avoid->getRadiusFractForMaxAvoidance(); //range [0,1]
						float remappedRadiusFrac = glm::clamp(radiusFrac - maxAvoidAtRadiusFrac, 0.f, 1.f); //makes a new range [0,1]
						remappedRadiusFrac /= (1.0f - maxAvoidAtRadiusFrac); //bring this back to a [0,1] range
						float avoidStrength = 1.f - remappedRadiusFrac;

						if (avoidStrength > 0.01f)
						{
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							// Project the velocity onto the vector to radius, this gives us a component of velocity that is going towards
							// the sphere center. We dampen this part of the velocity only. Effectively we trim out this part of the velocity
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							const vec3 toSphereCenter_n = glm::normalize(-toMe_v);
							float velocityProjection = glm::dot(toSphereCenter_n, * adjustVel_n);
							velocityProjection = glm::clamp(velocityProjection, 0.f, 1.f);	//clamp out velocity pointing AWAY from radius
							vec3 dampenVector_v = -(toSphereCenter_n * velocityProjection); //this logic oculd be simplified by moving -s around and just using toMe_v; but expresssed this way it may be more clear what is happening geometrically?

							dampenVector_v *= avoidStrength; //have a smooth dampening effect based on distance to radius
							vec3 dampenedVel_v = *adjustVel_n + dampenVector_v; //must normalize for next projection
							//don't make velocity a zero vector
							if (!Utils::float_equals(glm::length2(*adjustVel_n), 0.0f))
							{
								adjustVel_n = normalize(dampenedVel_v);

								//if this line below is flashing, it is likey we're generating zero vectors (not yet seen, but conciously putting this in branch so that can be indicated)
								if constexpr (constexpr bool bDebugToMeVec = false) { SpaceArcade::get().getDebugRenderSystem().renderLine(myXform.position, avoid->getWorldPosition(), 0.5f*color::metalicgold()); }
							}

							accumulatedStrength += avoidStrength;
						}
					}
				}
			}
		}

		if (accumulatedStrength < 0.001f)
		{
			adjustVel_n = std::nullopt;
		}

		if constexpr (bDebugAvoidance) 
		{ 
			if (adjustVel_n && bDrawAvoidance_debug) { debugRender_avoidance(accumulatedStrength); }
		}

		#endif //COMPILE_AVOIDANCE_VECTORS

		return adjustVel_n.has_value();
	}

	void Ship::handlePlacementDestroyed(const sp<GameEntity>& placement)
	{
		//unbind from this event so decrementing can never happen twice
		placement->onDestroyedEvent->removeAll(sp_this());

		activePlacements -= 1;
	}

#if COMPILE_CHEATS
	void Ship::cheat_OneShotPlacements()
	{
		auto cheatLambda = [this](std::vector<sp<ShipPlacementEntity>> placements)
		{
			for (sp<ShipPlacementEntity>& placement : placements)
			{
				if (HitPointComponent* hpComp = placement->getGameComponent<HitPointComponent>())
				{
					//#suggested perhaps cleaner to adjust it on the component and the entity will react the adjustments?
					placement->adjustHP(-hpComp->hp.current + 1);
				}
			}
		};
		cheatLambda(defenseEntities);
		cheatLambda(communicationEntities);
		cheatLambda(turretEntities);
	}

	void Ship::cheat_DestroyAllShipPlacements()
	{
		auto cheatLambda = [this](std::vector<sp<ShipPlacementEntity>> placements)
		{
			for (sp<ShipPlacementEntity>& placement : placements)
			{
				if (HitPointComponent* hpComp = placement->getGameComponent<HitPointComponent>())
				{
					//#suggested perhaps cleaner to adjust it on the component and the entity will react the adjustments?
					placement->adjustHP(-hpComp->hp.current);
				}
			}
		};
		cheatLambda(defenseEntities);
		cheatLambda(communicationEntities);
		cheatLambda(turretEntities);
	}

	void Ship::cheat_TurretsTargetPlayer()
	{
		using TargetType = TurretPlacement::TargetType;

		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (TargetType* playerShip = dynamic_cast<TargetType*>(player->getControlTarget()))
			{
				wp<TargetType> wpPlayerShip = playerShip->requestTypedReference_Nonsafe<TargetType>();
				for (const sp<ShipPlacementEntity>& turretBase : turretEntities)
				{
					if (TurretPlacement* turret = dynamic_cast<TurretPlacement*>(turretBase.get()))
					{
						turret->setTarget(wpPlayerShip);
					}
				}
			}
		}
	}

#endif //COMPILE_CHEATS

	void Ship::debugRender_avoidance(float accumulatedAvoidanceStrength) const
	{
		using namespace glm;

		float value = glm::clamp(accumulatedAvoidanceStrength, 0.f, 1.f);
		float max = 1.0f;
		float percFrac = value / max;
		renderPercentageDebugWidget(1.f, percFrac);
	}


	void Ship::ctor_configureObjectivePlacements()
	{
		using namespace glm;

		//called from ctor -- do not call virtuals from this function.
		assert(shipData);
		if (shipData && defenseEntities.size() == 0 && communicationEntities.size() == 0 && turretEntities.size() == 0)
		{
			ShipPlacementEntity::TeamData teamData;
			teamData.team = cachedTeamIdx;

			const std::vector<TeamData>& teams = shipData->getTeams();
			if (cachedTeamIdx < teams.size() && cachedTeamIdx >= 0)
			{
				TeamData loadedData = teams[cachedTeamIdx];
				teamData.shieldColor = loadedData.shieldColor;
				teamData.color = loadedData.teamTint;
				teamData.projectileColor = glm::clamp(loadedData.projectileColor + vec3(0.25f), vec3(0), vec3(1.f));
				teamData.primaryProjectile = primaryProjectile;

				if (!teamData.primaryProjectile)
				{
					//#TODO replace this with some default config available statically - 
					//find the first available projectile config and use that
					if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
					{
						for (auto projectileConfigIter : activeMod->getProjectileConfigs())
						{
							if (projectileConfigIter.second)
							{
								teamData.primaryProjectile = projectileConfigIter.second;
								break;
							}
						}
					}
				}
			}
			
			auto processPlacements = [&](const std::vector<PlacementSubConfig>& typedConfigs, std::vector<sp<ShipPlacementEntity>>& outputContainer)
			{
				for (const PlacementSubConfig& placementConfig : typedConfigs)
				{
					sp<ShipPlacementEntity> newPlacement = nullptr;
					if (placementConfig.placementType == PlacementType::COMMUNICATIONS)
					{
						newPlacement = new_sp<CommunicationPlacement>();
					} 
					else if (placementConfig.placementType == PlacementType::DEFENSE)
					{
						newPlacement = new_sp<DefensePlacement>();
					}
					else if (placementConfig.placementType == PlacementType::TURRET)
					{
						newPlacement = new_sp<TurretPlacement>();
					}
					else
					{
						log(__FUNCTION__, LogLevel::LOG_ERROR, "failed to find subtype of serialized placement");
					}

					if (newPlacement)
					{
						newPlacement->replacePlacementConfig(placementConfig, *shipData);
						newPlacement->setTeamData(teamData);
						outputContainer.push_back(newPlacement);
					}
				}
			};

			processPlacements(shipData->getDefensePlacements(), defenseEntities);
			processPlacements(shipData->getCommuncationPlacements(), communicationEntities);
			processPlacements(shipData->getTurretPlacements(), turretEntities);

			activePlacements = defenseEntities.size() + communicationEntities.size() + turretEntities.size();
		}
	}

	void Ship::renderPercentageDebugWidget(float rightOffset, float percFrac) const
	{
		using namespace glm;
		//param percFrac is on range [0,1]

		static DebugRenderSystem& debugRenderer = GameBase::get().getDebugRenderSystem();
		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();

		const Transform& shipXform = getTransform();
		vec3 up_n = vec3(getUpDir());
		vec3 right_n = vec3(getRightDir());
		if (const sp<CameraBase> camera = playerSystem.getPlayer(0)->getCamera())
		{
			up_n = camera->getUp();
			right_n = camera->getRight();
		}
		vec3 w_n = normalize(cross(right_n, up_n));

		vec3 basePnt = shipXform.position + right_n * rightOffset;
		vec3 maxPnt = basePnt + up_n * 2.f;
		vec3 toMax = maxPnt - basePnt;

		toMax *= percFrac; 
		float curLen = glm::length(toMax);

		//render total area that cube can occupy
		debugRenderer.renderLine(basePnt, maxPnt, vec3(0, 1.f, 0));
			
		//render cube occupying area of line
		mat4 cubeRotation{ vec4(right_n, 0), vec4(up_n, 0), vec4(w_n,0), vec4(vec3(0.f),1.f)};
		//cubeRotation = transpose(cubeRotation);
		mat4 cubeModel{ 1.f };
		cubeModel = glm::translate(cubeModel, basePnt); //position cube at base point
		cubeModel = glm::translate(cubeModel, (curLen / 2.f)* up_n); //reposition unit cube's center to be halfway along vector
		cubeModel = cubeModel * cubeRotation;
		cubeModel = glm::scale(cubeModel, vec3(1.f,curLen,1.f));
		debugRenderer.renderCube(cubeModel, vec3(percFrac, 0, 1.f - percFrac));
	}

	bool Ship::bRenderAvoidanceSpheres = false;

}
