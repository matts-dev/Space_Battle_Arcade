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

namespace
{
	using namespace SA;

	sp<ShieldEffect::ParticleCache> shieldEffects = new_sp<ShieldEffect::ParticleCache>();
}


namespace SA
{
	Ship::Ship(
		const sp<Model3D>& model,
		const Transform& spawnTransform,
		const sp<CollisionInfo>& inCollisionData
	)
		: RenderModelEntity(model, spawnTransform),
		collisionData(inCollisionData),
		constViewCollisionData(collisionData)
	{
		overlappingNodes_SH.reserve(10);

		glm::vec4 pointingDir{ 0,0,1,0 };
		glm::mat4 rotMatrix = glm::toMat4(spawnTransform.rotQuat);

		glm::vec4 rotDir = rotMatrix * pointingDir;

		velocityDir_n = glm::normalize(rotDir);

		//staticInitCheck();
	}

	Ship::Ship(const SpawnData& spawnData)
		: RenderModelEntity(spawnData.spawnConfig->getModel(), spawnData.spawnTransform),
		collisionData(spawnData.spawnConfig->toCollisionInfo()), 
		constViewCollisionData(collisionData),
		cachedTeamIdx(spawnData.team)
	{
		overlappingNodes_SH.reserve(10);
		primaryProjectile = spawnData.spawnConfig->getPrimaryProjectileConfig();
		shieldOffset = spawnData.spawnConfig->getShieldOffset();
		shipData = spawnData.spawnConfig;

		createGameComponent<BrainComponent>();
		createGameComponent<TeamComponent>();
		energyComp = createGameComponent<ShipEnergyComponent>();
		if (TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			//bound to delegate in post construct
			teamComp->setTeam(spawnData.team);
			updateTeamDataCache();
		}
	}

	Ship::~Ship()
	{
		//#TODO create LP bindings for delegates. Have ships ticked via LP ticker. Draw debug box for LP ticker to quickly spot meory leaks when they happen. #memory_leaks
		//adding for debug 
#define LOG_SHIP_DTOR 0
#if LOG_SHIP_DTOR 
		log("logShip", LogLevel::LOG, "ship destroyed");
#endif 
	}

	const sp<const CollisionInfo>& Ship::getCollisionInfo() const
	{
		//This will actually make a copy of the sp if we return collision data directly
		//So, optimizing by storing a shared pointer that views owning data as const and returning that by reference
		return constViewCollisionData;
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

	glm::vec3 Ship::getVelocity()
	{
		//apply a speed increase if you have a full tank of energy (this helps create distance when two ai's are dogfighting)
		constexpr float energyThresholdForBoost = 75.f;
		float highEnergyBoost = glm::clamp(energyComp->getEnergy() / 75.f, 1.0f, 1.1f);

		return velocityDir_n 
			* currentSpeedFactor * getMaxSpeed() * speedGamifier
			* highEnergyBoost 
			* adjustedBoost;
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
		
			assert(teams.size() > 0);
			cachedTeamData = teams[cachedTeamIdx];
		}

	}

	void Ship::handleTeamChanged(size_t oldTeamId, size_t newTeamId)
	{
		updateTeamDataCache();
	}

	void Ship::draw(Shader& shader)
	{
		shader.setUniform3f("objectTint", cachedTeamData.teamTint);
		RenderModelEntity::draw(shader);
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

	void Ship::postConstruct()
	{
		rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

		//WARNING: caching world sp will create cyclic reference
		if (LevelBase* world = getWorld())
		{
			Transform xform = getTransform();
			glm::mat4 xform_m = xform.getModelMatrix();
			//collisionHandle = world->getWorldGrid().insert(*this, getWorldOBB(xform_m));
			collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB()); 
		}
		else
		{
			throw std::logic_error("World entity being created but there is no containing world");
		}

		if(TeamComponent* team = getGameComponent<TeamComponent>())
		{
			team->onTeamChanged.addWeakObj(sp_this(), &Ship::handleTeamChanged);
		}
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
		Transform xform = getTransform();
		xform.position += getVelocity() * dt_sec;
		glm::mat4 movedXform_m = xform.getModelMatrix();

		NAN_BREAK(xform.position);

		//update collision data
		collisionData->updateToNewWorldTransform(movedXform_m);

		//update the spatial hash
		LevelBase* world = getWorld();
		if (world && collisionHandle)
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
			//worldGrid.updateEntry(collisionHandle, getWorldOBB(xform.getModelMatrix()));
			worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());

			//test if collisions occurred
			overlappingNodes_SH.clear();
			worldGrid.lookupNodesInCells(*collisionHandle, overlappingNodes_SH);
			for (sp <SH::GridNode<WorldEntity>> node : overlappingNodes_SH)
			{
				//#TODO make sure this object's shape has been updated to transform! this should be done before the loop
				//#TODO for each node, get their shape and do a collision tests
				//#TODO if collision, make sure this object's SAT::Shapes are updated
			}
#if SA_CAPTURE_SPATIAL_HASH_CELLS
			SpatialHashCellDebugVisualizer::appendCells(worldGrid, *collisionHandle);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
		}

		setTransform(xform);

		if (!activeShieldEffect.expired())
		{
			//locking wp may be slow as it requires atomic reference count increment; may need to use soft-ptrs if I make that system
			sp<ActiveParticleGroup> activeShield_sp = activeShieldEffect.lock();
			activeShield_sp->xform.rotQuat = xform.rotQuat;
			activeShield_sp->xform.position = xform.position;
			//offset for non-centered scaling issues
			activeShield_sp->xform.position += glm::vec3(rotateLocalVec(glm::vec4(shieldOffset, 0.f))); //#optimize rotating dir is expensive; perhaps cache with dirty flag?
		}
#define EXTRA_SHIP_DEBUG_INFO 0
#if EXTRA_SHIP_DEBUG_INFO
		{
			DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
			debug.renderCube(movedXform_m, color::brightGreen());
		}
#endif
	} 

	//const std::array<glm::vec4, 8> Ship::getWorldOBB(const glm::mat4 xform) const
	//{
	//	const std::array<glm::vec4, 8>& localAABB = collisionData->getLocalAABB();
	//	const std::array<glm::vec4, 8> OBB =
	//	{
	//		xform * localAABB[0],
	//		xform * localAABB[1],
	//		xform * localAABB[2],
	//		xform * localAABB[3],
	//		xform * localAABB[4],
	//		xform * localAABB[5],
	//		xform * localAABB[6],
	//		xform * localAABB[7],
	//	};
	//	return OBB;
	//}

	void Ship::notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{
		if (cachedTeamIdx != hitProjectile.team)
		{
			//#BUG something is keeping ships alive after they are destroyed (eg holding on to a shared ptr)
			//changing references to #releasing_ptr is the best fix, but for now only spawning particle if not destroyed
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
				if (!activeShieldEffect.expired())
				{
					sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
					shieldEffect_sp->resetTimeAlive();
				}
				else
				{
					if (!activeShieldEffect.expired())
					{
						sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
						//#TODO kill the shield effect
					}

					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = shieldEffects->getEffect(getMyModel(), cachedTeamData.shieldColor);
					const Transform& shipXform = this->getTransform(); 
					particleSpawnParams.xform.position = shipXform.position;
					particleSpawnParams.xform.position += glm::vec3(rotateLocalVec(glm::vec4(shieldOffset, 0.f)));
					particleSpawnParams.xform.rotQuat = shipXform.rotQuat;
					particleSpawnParams.xform.scale = shipXform.scale * 1.1f;  //scale up to see effect around ship

					activeShieldEffect = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
				}
			}
		}
	}

}
