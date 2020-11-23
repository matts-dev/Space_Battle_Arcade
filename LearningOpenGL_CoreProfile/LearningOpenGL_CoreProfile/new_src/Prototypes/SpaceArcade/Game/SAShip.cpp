#include "SAShip.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAGameEntity.h"
#include "GameSystems/SAProjectileSystem.h"
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
#include "Cheats/SpaceArcadeCheatSystem.h"
#include "SAShipPlacements.h"
#include "GameSystems/SAModSystem.h"
#include "Components/FighterSpawnComponent.h"
#include <type_traits>
#include "UI/GameUI/Widgets3D/Widget3D_Ship.h"
#include "../Tools/DataStructures/AdvancedPtrs.h"
#include "AI/GlobalSpaceArcadeBehaviorTreeKeys.h"
#include "GameModes/ServerGameMode_SpaceBase.h"
#include "../Tools/PlatformUtils.h"
#include "../GameFramework/SAAudioSystem.h"
#include "../Rendering/Lights/PointLight_Deferred.h"
#include "../GameFramework/SARenderSystem.h"
#include "SAPlayer.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Runtime flags
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool bDrawAvoidance_debug = false;
	bool bForcePlayerAvoidance_debug = false;
	float targetPlayerThresholdDist2 = std::powf(20.f, 2.f);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// statics 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*static*/ bool Ship::bRenderAvoidanceSpheres = false;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// methods
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Ship::Ship(const SpawnData& spawnData)
		: RenderModelEntity(spawnData.spawnConfig->getModel(), spawnData.spawnTransform),
		collisionData(spawnData.spawnConfig->toCollisionInfo()),
		cachedTeamIdx(spawnData.team)
	{
		if (spawnData.bEditorMode)
		{
			bEditorMode = true;
			configureForEditorMode(spawnData);
			return;
		}

		shipConfigData = spawnData.spawnConfig;
		overlappingNodes_SH.reserve(10);
		primaryProjectile = spawnData.spawnConfig->getPrimaryProjectileConfig();
		bCollisionReflectForward = shipConfigData->getCollisionReflectForward();

		ctor_configureObjectivePlacements();

		////////////////////////////////////////////////////////
		// Make components
		////////////////////////////////////////////////////////
		createGameComponent<BrainComponent>();
		TeamComponent* teamComp = createGameComponent<TeamComponent>();
		hpComp = createGameComponent<HitPointComponent>();
		CollisionComponent* collisionComp = createGameComponent<CollisionComponent>();
		energyComp = createGameComponent<ShipEnergyComponent>();
		if (spawnData.spawnConfig
			&& spawnData.spawnConfig->getSpawnPoints().size() > 0
			&& spawnData.spawnConfig->getSpawnableConfigsByName().size() > 0)
		{
			//only creating this if it will be used makes debugging easier
			fighterSpawnComp = createGameComponent<FighterSpawnComponent>(); //perhaps shouldn't cache this? right now it is owned by shared ptr which will have some overhead for access; should be unique ptr once flow for postConstruct is defined there too (if possible)
		}

		////////////////////////////////////////////////////////
		// any extra component configuration
		////////////////////////////////////////////////////////
		teamComp->setTeam(spawnData.team);
		updateTeamDataCache();
		collisionComp->setCollisionData(collisionData);
		collisionComp->setKinematicCollision(spawnData.spawnConfig->requestCollisionTests());
		hpComp->overwriteHP(HitPoints{ /*current*/100.f, /*max*/100.f });
		if (fighterSpawnComp)
		{
			if (spawnData.spawnConfig) { fighterSpawnComp->loadSpawnPointData(*spawnData.spawnConfig); }
			fighterSpawnComp->setTeamIdx(cachedTeamIdx);
			fighterSpawnComp->setPostSpawnCustomization(
				[](const sp<Ship>& spawned)
				{
					//spawned->spawnNewBrain<FlyInDirectionBrain>(); 
					//spawned->spawnNewBrain<DogfightTestBrain_VerboseTree>();
					//spawned->spawnNewBrain<WanderBrain>();
					//spawned->spawnNewBrain<EvadeTestBrain>();
					//spawned->spawnNewBrain<DogfightTestBrain>();
					spawned->spawnNewBrain<FighterBrain>();
				}
			);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// AUDIO
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		AudioSystem& audioSystem = GameBase::get().getAudioSystem();

		auto configureSoundEmitter = [&audioSystem](sp<AudioEmitter>& emitter, const SoundEffectSubConfig& soundConfig, AudioEmitterPriority priority = AudioEmitterPriority::GAMEPLAY_AMBIENT)
		{
			if (soundConfig.assetPath.size() > 0)
			{
				emitter = audioSystem.createEmitter();
				soundConfig.configureEmitter(emitter);
				emitter->setPriority(priority);
			}
		};

		//wait to tick sounds so that we do that after we've already generated all sounds
		configureSoundEmitter(sfx_engine, shipConfigData->getConfig_sfx_engineLoop());
		configureSoundEmitter(sfx_muzzle, shipConfigData->getConfig_sfx_muzzle());
		configureSoundEmitter(sfx_explosion, shipConfigData->getConfig_sfx_explosion(), AudioEmitterPriority::GAMEPLAY_COMBAT);


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// activate sounds after we have configured them
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		tickSounds(); //do an initial tick to apply position data to sounds before we activate them, this way we don't hear a blip on spawn as they're in the location location
		auto activateSound = [](const sp<AudioEmitter>& emitter) { if (emitter) { emitter->play(); }};
		activateSound(sfx_engine);
		
		//regenerateEngineVFX(); //initial spawn isn't spawning particle... hmm
	}

	void Ship::postConstruct()
	{
		if (bEditorMode)
		{
			return;
		}

		rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

		for (const AvoidanceSphereSubConfig& sphereConfig : shipConfigData->getAvoidanceSpheres())
		{
			//must wait for postConstruct because we need to pass sp_this() for owner field.
			sp<AvoidanceSphere> avoidanceSphere = new_sp<AvoidanceSphere>(sphereConfig.radius, sphereConfig.localPosition, sp_this());
			avoidanceSpheres.push_back(avoidanceSphere);
		}

		projectileFireLocationOffsets = shipConfigData->getFireLocationOffsets();

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

		postctor_configureObjectivePlacements();
		for (const sp<ShipPlacementEntity>& placement : communicationEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }
		for (const sp<ShipPlacementEntity>& placement : generatorEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }
		for (const sp<ShipPlacementEntity>& placement : turretEntities) { placement->onDestroyedEvent->addWeakObj(sp_this(), &Ship::handlePlacementDestroyed); }

		activeGenerators = generatorEntities.size();
		activeTurrets = turretEntities.size();
		activeCommunications = communicationEntities.size();

		if (fighterSpawnComp)
		{
			fighterSpawnComp->onSpawnedEntity.addWeakObj(sp_this(), &Ship::handleSpawnedEntity);
		}
		if (hpComp)
		{
			hpComp->onHpChangedEvent.addWeakObj(sp_this(), &Ship::handleHpAdjusted);
		}

		regenerateEngineVFX(); //needs to come after updating team data cache if we're using team colors for fire

#if COMPILE_SHIP_WIDGET
		shipWidget = new_sp<Widget3D_Ship>();
		shipWidget->setOwnerShip(sp_this());
#endif //COMPILE_SHIP_WIDGET

#if COMPILE_CHEATS
		SpaceArcadeCheatSystem& cheatSystem = static_cast<SpaceArcadeCheatSystem&>(GameBase::get().getCheatSystem());
		cheatSystem.oneShotShipObjectivesCheat.addWeakObj(sp_this(), &Ship::cheat_oneShotPlacements);
		cheatSystem.destroyAllShipObjectivesCheat.addWeakObj(sp_this(), &Ship::cheat_destroyAllShipPlacements);
		if (turretEntities.size() > 0) cheatSystem.turretsTargetPlayerCheat.addWeakObj(sp_this(), &Ship::cheat_turretsTargetPlayer);
		if (generatorEntities.size() > 0) cheatSystem.destroyAllGeneratorsCheat.addWeakObj(sp_this(), &Ship::cheat_destroyAllGenerators);
		if (communicationEntities.size() > 0) cheatSystem.commsTargetPlayerCheat.addWeakObj(sp_this(), &Ship::cheat_commsTargetPlayer);
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
		destroyEngineVFX();

		if (sfx_engine)
		{
			sfx_engine->stop();
		}
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
		return glm::pi<float>() * glm::clamp(1.f / (currentSpeedFactor + 0.0001f), 0.f, 3.f);
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

	void Ship::setOwningSpawnComponent(fwp<FighterSpawnComponent>& newOwningSpawnComp)
	{
		this->owningSpawnComponent = newOwningSpawnComp;
	}

	void Ship::setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig)
	{
		primaryProjectile = projectileConfig;
	}

	void Ship::playerMuzzleSFX()
	{
		if (sfx_muzzle)
		{
			bool bIsPlayer = false;
			if (OwningPlayerComponent* playerComp = getGameComponent<OwningPlayerComponent>())
			{
				bIsPlayer = playerComp->hasOwningPlayer();
			}
			sfx_muzzle->setPriority(bIsPlayer ? AudioEmitterPriority::GAMEPLAY_PLAYER : AudioEmitterPriority::GAMEPLAY_COMBAT); //make sure we can hear it if it is from player
			sfx_muzzle->setPosition(getWorldPosition());
			sfx_muzzle->setVelocity(getVelocity());
			sfx_muzzle->stop(); //clear any previous muzzle sound; we may need have multiple emitters to handle this if it sounds bad
			sfx_muzzle->play();
		}
	}

	void Ship::updateTeamDataCache()
	{
		//#TODO perhaps just cache temp comp directly?

		if (TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			cachedTeamIdx = teamComp->getTeam();
			const std::vector<TeamData>& teams = shipConfigData->getTeams();

			if (cachedTeamIdx >= teams.size()) { cachedTeamIdx = 0; }

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
				applyTeamToObjectives(generatorEntities, cachedTeamIdx);
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

	//void Ship::onLevelRender()
	//{
	//	CustomGameShaders& gameCustomShaders = SpaceArcade::get().getGameCustomShaders();
	//	if (gameCustomShaders.forwardModelShader)
	//	{
	//		render(*gameCustomShaders.forwardModelShader);
	//	}
	//}

	void Ship::render(Shader& shader)
	{
		glm::mat4 configuredModelXform = collisionData->getRootXform(); //#TODO #REFACTOR this ultimately comes from the spawn config, it is somewhat strange that we're reading this from collision data.But we need this to render models to scale.
		glm::mat4 rawModel = getTransform().getModelMatrix();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(rawModel * configuredModelXform)); //unfortunately the spacearcade game is setting this uniform, so we're hitting this hot code twice.
		shader.setUniform3f("objectTint", cachedTeamData.teamTint);
		RenderModelEntity::render(shader);

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
					placement->render(shader);
				}
			}
		};
		renderPlacements(generatorEntities, shader);
		renderPlacements(communicationEntities, shader);
		renderPlacements(turretEntities, shader);
	}

	void Ship::onDestroyed()
	{
		RenderModelEntity::onDestroyed();
		collisionHandle = nullptr; //release spatial hashing information

#if ENABLE_BANDAID_FIXES
		//#BUG #TODO it appears spatial hash is not getting cleaned up properly in case of carrier ships; repro is enable camera debugging, destroy carrier, fly to center and see camera is colliding
		//for now, just removing collision comp so it cannot be used to do collision
		if (CollisionComponent* collisionComp = getGameComponent<CollisionComponent>())
		{
			collisionComp->setKinematicCollision(false);
		}
#endif //ENABLE_BANDAID_FIXES
		//#TODO #BUG avoidance spheres still affecting AI after carrier ship destroyed
		//#TODO #BUG cleanup placements after carrier is destroyed

		destroyEngineVFX();
	}

	void Ship::onPlayerControlTaken(const sp<PlayerBase>& player)
	{
		if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
		{
			if (AIBrain* brain = brainComp->getBrain())
			{
				brain->sleep();
			}
		}

		if (OwningPlayerComponent* playerComp = createOrGetOptionalGameComponent<OwningPlayerComponent>(*this))
		{
			playerComp->setOwningPlayer(player);
		}

		if (sfx_engine){sfx_engine->setPriority(AudioEmitterPriority::GAMEPLAY_PLAYER);}
		if (sfx_explosion){sfx_explosion->setPriority(AudioEmitterPriority::GAMEPLAY_PLAYER);}
		if (sfx_muzzle) {sfx_muzzle->setPriority(AudioEmitterPriority::GAMEPLAY_PLAYER); }

		bEnableAvoidanceFields = false || bForcePlayerAvoidance_debug;
		bAwakeBrainAfterStasis = false;
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

		if (OwningPlayerComponent* playerComp = getGameComponent<OwningPlayerComponent>())
		{
			playerComp->setOwningPlayer(nullptr);
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

	WorldEntity* Ship::asWorldEntity()
	{
		return this;
	}

	PointLight_Deferred::UserData makeProjectileLightData(TeamData& cachedTeamData)
	{
		PointLight_Deferred::UserData data;
		data.diffuseIntensity = cachedTeamData.projectileColor;
		return data;
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
			setProjectileStartPoint(spawnData.start, spawnData.traceStart, spawnData.direction_n);
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.sfx = shipConfigData->getConfig_sfx_projectileLoop();
			spawnData.owner = sp_this();
			spawnData.projectileLightData = makeProjectileLightData(cachedTeamData);
			playerMuzzleSFX();
			projectileSys->spawnProjectile(spawnData, *primaryProjectile); 

		}
	}

	void Ship::fireProjectileInDirection(glm::vec3 dir_n)
	{
		using namespace glm;

		if (primaryProjectile && length2(dir_n) > 0.001 && FIRE_PROJECTILE_ENABLED)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();
			
			ProjectileSystem::SpawnData spawnData;
			//#TODO #scenenodes doesn't account for parent transforms
			spawnData.direction_n = glm::normalize(dir_n);
			setProjectileStartPoint(spawnData.start, spawnData.traceStart, spawnData.direction_n);
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.sfx = shipConfigData->getConfig_sfx_projectileLoop();
			spawnData.owner = sp_this();
			spawnData.projectileLightData = makeProjectileLightData(cachedTeamData);

			playerMuzzleSFX();
			projectileSys->spawnProjectile(spawnData, *primaryProjectile);
		}
	}

	void Ship::setProjectileStartPoint(glm::vec3& outProjectileStart, std::optional<glm::vec3>& traceStart, const glm::vec3& dir_n)
	{
		using namespace glm;

		constexpr bool bEnableProjectileSpawnOffsets = true;

		if (projectileFireLocationOffsets.size() == 0 || !bEnableProjectileSpawnOffsets)
		{
			outProjectileStart = dir_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
		}
		else
		{
			const Transform& shipXform = getTransform();
			vec3 forward_n = vec3(getForwardDir());

			fireLocationIndex = (fireLocationIndex + 1) % projectileFireLocationOffsets.size(); //wrap around index.
			glm::vec3 offset = projectileFireLocationOffsets[fireLocationIndex];
			outProjectileStart = shipXform.position + forward_n*offset.z + vec3(getRightDir())*offset.x + vec3(getUpDir())*offset.y;

			//find trace start point
			glm::vec3 toProjStart_v = outProjectileStart - shipXform.position;
			glm::vec3 projOnToForward_v = Utils::project(toProjStart_v, forward_n);
			traceStart = shipXform.position + projOnToForward_v;
		}
	}

	void Ship::tickVFX()
	{
		tickShieldFX();
		tickEngineFX();
	}

	void Ship::enterSpawnStasis()
	{
		if (LevelBase* world = getWorld())
		{
			if (const sp<TimeManager>& worldTimeManager = world->getWorldTimeManager())
			{
				spawnStasisTimerDelegate = new_sp<MultiDelegate<>>();
				spawnStasisTimerDelegate->addWeakObj(sp_this(), &Ship::handleSpawnStasisOver);
				worldTimeManager->createTimer(spawnStasisTimerDelegate, 5.f);

				BrainComponent* brainComp = getGameComponent<BrainComponent>();
				if (brainComp && brainComp->brain)
				{
					bAwakeBrainAfterStasis = brainComp->brain->isActive();
					brainComp->brain->sleep();
				}
			}
		}
	}

	void Ship::moveTowardsPoint(const glm::vec3& moveLoc, float dt_sec, float speedAmplifier, bool bRoll, float turnAmplifier, float viscosity)
	{
		MoveTowardsPointArgs args{ moveLoc, dt_sec };
		args.speedMultiplier = speedAmplifier;
		args.bRoll = bRoll;
		args.turnMultiplier = turnAmplifier;
		args.viscosity = viscosity;
		moveTowardsPoint(args);
	}

	void Ship::moveTowardsPoint(const Ship::MoveTowardsPointArgs& p)
	{
		//#TODO #REFACTOR #cleancode #input_vector this should in influencing an input vector, rather than directly influencing the velocity; tick should do the visual updates and velocity changes based on input vector
		using namespace glm;

		const Transform& xform = getTransform();
		vec3 targetDir = p.moveLoc - xform.position;

		vec3 forwardDir_n = glm::normalize(vec3(getForwardDir()));
		vec3 targetDir_n = glm::normalize(targetDir);
		bool bPointingTowardsMoveLoc = glm::dot(forwardDir_n, targetDir_n) >= 0.999f;

		if (!bPointingTowardsMoveLoc)
		{
			vec3 rotationAxis_n = glm::cross(forwardDir_n, targetDir_n);

			float angleBetween_rad = Utils::getRadianAngleBetween(forwardDir_n, targetDir_n);
			float maxTurn_Rad = getMaxTurnAngle_PerSec() * p.turnMultiplier * p.dt_sec;
			float possibleRotThisTick = glm::clamp(maxTurn_Rad / angleBetween_rad, 0.f, 1.f);

			//slow down turn if some viscosity is being applied
			float fluidity = glm::clamp(1.f - p.viscosity, 0.f, 1.f);
			possibleRotThisTick *= fluidity;

			quat completedRot = Utils::getRotationBetween(forwardDir_n, targetDir_n) * xform.rotQuat;
			quat newRot = glm::slerp(xform.rotQuat, completedRot, possibleRotThisTick);

			//roll ship -- we want the ship's right (or left) vector to match this rotation axis to simulate it pivoting while turning
			vec3 rightVec_n = newRot * vec3(1, 0, 0);
			bool bRollMatchesTurnAxis = glm::dot(rightVec_n, rotationAxis_n) >= 0.99f;

			vec3 newForwardVec_n = glm::normalize(newRot * vec3(localForwardDir_n()));
			if (!bRollMatchesTurnAxis && p.bRoll)
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
			speedGamifier = p.speedMultiplier;
		}
		else
		{
			setVelocityDir(forwardDir_n);
			speedGamifier = p.speedMultiplier;
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

	bool Ship::fireProjectileAtShip(const WorldEntity& myTarget, std::optional<float> inFireRadius_cosTheta /*=empty*/, float shootRandomOffsetStrength /*= 1.f*/)
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
		if (bEditorMode) {return;}

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

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// handle VFX
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const Transform& xform = getTransform();

		tickVFX();

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// avoidance spheres
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//does not include the spawn config transform (ie the size up for carrier ships)
		glm::mat4 modelMatrix = xform.getModelMatrix(); //this needs to be done after tick kinematic is complete so it reflects the new position
		if (avoidanceSpheres.size() > 0)
		{
			for (sp<AvoidanceSphere>& myAvoidSphere : avoidanceSpheres)
			{
				myAvoidSphere->setParentXform(modelMatrix);
			}
		}

		std::optional<glm::mat4> fullParentXform;
		//////////////////////////////////////////////////////////
		// placements - must be handled after updates to position
		//////////////////////////////////////////////////////////
		//#todo #scenenodes no need to pass parent xforms if scene node hierarchy is used
		if (hasObjectives())
		{
			glm::mat4 configXform = collisionData->getRootXform();
			fullParentXform = fullParentXform.has_value() ? fullParentXform : modelMatrix * configXform; //needs to be done after tick kinematic

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
			tickPlacements(dt_sec, generatorEntities, *fullParentXform);
			tickPlacements(dt_sec, communicationEntities, *fullParentXform);
			tickPlacements(dt_sec, turretEntities, *fullParentXform);
		}

		////////////////////////////////////////////////////////
		// spawning
		////////////////////////////////////////////////////////
		if (fighterSpawnComp)
		{
			fullParentXform = fullParentXform.has_value() ? fullParentXform : modelMatrix * collisionData->getRootXform(); //needs to be done after tick kinematic
			fighterSpawnComp->updateParentTransform(*fullParentXform);
			fighterSpawnComp->tick(dt_sec);
		}


		tickSounds();
	} 

	void Ship::TryTargetPlayer()
	{
		if(bool bIsServer = true)
		{
			bool bCleanNullPlayers = false;

			for (size_t playerIdx = 0; playerIdx < ServerGameMode_SpaceBase::playersNeedingTarget.size(); ++playerIdx)
			{
				if (const fwp<PlayerBase>& player = ServerGameMode_SpaceBase::playersNeedingTarget[playerIdx])
				{
					//guessing that calling virtual to check distance will be faster than accessing team component and comparing teams
					IControllable* controlTarget = player->getControlTarget();
					if (WorldEntity* playerControlTarget_WE = (controlTarget) ? controlTarget->asWorldEntity() : nullptr)
					{
						float distToPlayer2 = glm::distance2(playerControlTarget_WE->getWorldPosition(), getWorldPosition());
						if (distToPlayer2 < targetPlayerThresholdDist2 && this != controlTarget)
						{
							TeamComponent* playerTeamComp = playerControlTarget_WE->getGameComponent<TeamComponent>();
							if (playerTeamComp && playerTeamComp->getTeam() != getTeam())
							{
								//player should have already met targeting requirements as it shouldn't have been in container of players needing target
								//worst case is player gets two ships targeting it.
								BrainComponent* myBrainComp = getGameComponent<BrainComponent>();
								if (const BehaviorTree::Tree* tree = myBrainComp ? myBrainComp->getTree() : nullptr)
								{
									BehaviorTree::Memory& memory = tree->getMemory();

									sp<WorldEntity> playerWE = playerControlTarget_WE->requestTypedReference_Nonsafe<WorldEntity>().lock();
									memory.replaceValue(BT_TargetKey, playerWE);
								}

								ServerGameMode_SpaceBase::playersNeedingTarget[playerIdx] = nullptr; //null this since we don't want other ships to try and target player now that it has a target
								bCleanNullPlayers = true;
								break;
							}
						}
					}
				}
				else
				{
					bCleanNullPlayers = true;
				}
			}
			if (bCleanNullPlayers)
			{
				auto newEndIter = std::remove_if(ServerGameMode_SpaceBase::playersNeedingTarget.begin(), ServerGameMode_SpaceBase::playersNeedingTarget.end(),
					[this](const fwp<PlayerBase>& player){
						return !bool(player); //if player is null, remove it.
					});
				ServerGameMode_SpaceBase::playersNeedingTarget.erase(newEndIter, ServerGameMode_SpaceBase::playersNeedingTarget.end());
			}
		}
	}

	bool Ship::isCarrierShip() const
	{
		//return getGameComponent<FighterSpawnComponent>() != nullptr;
		return fighterSpawnComp != nullptr;
	}

	sp<SA::ShipPlacementEntity> Ship::getRandomObjective()
	{
		size_t containerChoice = rng->getInt<size_t>(0, 2);

		decltype(generatorEntities)* targetContainer = nullptr; 

		if (containerChoice == uint8_t(PlacementType::COMMUNICATIONS) && activeCommunications> 0)
		{
			targetContainer = &communicationEntities;
		}
		else if (containerChoice == uint8_t(PlacementType::DEFENSE) && activeGenerators > 0)
		{
			targetContainer = &generatorEntities;
		}
		else if (containerChoice == uint8_t(PlacementType::TURRET) && activeTurrets> 0)
		{
			targetContainer = &turretEntities;
		}

		//choice did not have any valid objectives, try to find one that does
		if (targetContainer == nullptr)
		{
			if (activeCommunications > 0)
			{
				targetContainer = &communicationEntities;
			}
			else if (activeGenerators > 0)
			{
				targetContainer = &generatorEntities;
			}
			else
			{
				targetContainer = &turretEntities;
			}
		}

		//if we still don't have a objective container, then carrier has no more objectives left to be destroyed
		if (targetContainer)
		{
			size_t idx = rng->getInt<size_t>(0, targetContainer->size() - 1);
			if (Utils::isValidIndex(*targetContainer, idx))
			{
				//walk all entities just in case the one we chose was nullptr. start at chosen idx and wrap around
				for (size_t walkCount = 0; walkCount < targetContainer->size(); ++walkCount)
				{
					const sp<ShipPlacementEntity>& objective = (*targetContainer)[idx];
					if (objective && !objective->isPendingDestroy())
					{
						return objective; //early out of the loop
					}

					idx = (idx + 1) % targetContainer->size(); //wrap around idx
				}

			} else {STOP_DEBUGGER_HERE(); } //this should never happen -- what happened?
		}

		return nullptr;
	}

	void Ship::requestSpawnFighterAgainst(const sp<WorldEntity>& attacker)
	{
		if (attacker && fighterSpawnComp)
		{
			if (sp<Ship> defenderShip = fighterSpawnComp->spawnEntity())
			{
				ShipUtilLibrary::setShipTarget(defenderShip, attacker);
			}
		}
	}

	void Ship::setAvoidanceSensitivity(float newValue)
	{
		avoidanceSensitivity = glm::clamp(newValue, 0.f, 1.f);
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

	void Ship::tickSounds()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// handle sound
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (sfx_engine)
		{
			sfx_engine->setPosition(getWorldPosition());
			sfx_engine->setVelocity(getVelocity());
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
				transientCollidingProjectile = &hitProjectile; //todo, perahps a scoped updater so this is automatic and immune to refactor mistakes?
				hpComp->adjust(-float(hitProjectile.damage));
				transientCollidingProjectile = nullptr;

				//ship::handleAdjustHp will play hp related effects!
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
			float hdrFactor = GameBase::get().getRenderSystem().isUsingHDR() ? 3.f : 1.f; //@hdr_tweak

			ParticleSystem::SpawnParams particleSpawnParams;
			particleSpawnParams.particle = SharedGFX::get().shieldEffects_ModelToFX->getEffect(getMyModel(), cachedTeamData.shieldColor * hdrFactor);
			const Transform& shipXform = this->getTransform();
			particleSpawnParams.xform.position = shipXform.position;
			particleSpawnParams.xform.rotQuat = shipXform.rotQuat;
			particleSpawnParams.xform.scale = shipXform.scale;

			//#TODO #REFACTOR hacky as only considering scale. particle perhaps should use matrices to avoid this, or have list of transform to apply.
			//making the large ships show correct effect. Perhaps not even necessary.
			Transform modelXform = shipConfigData->getModelXform();
			particleSpawnParams.xform.scale *= modelXform.scale;

			activeShieldEffect = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
		}
	}

	void Ship::tickShieldFX()
	{
		const Transform& xform = getTransform();
		if (!activeShieldEffect.expired())
		{
			//locking wp may be slow as it requires atomic reference count increment; may need to use soft-ptrs if I make that system
			sp<ActiveParticleGroup> activeShield_sp = activeShieldEffect.lock();
			activeShield_sp->xform.rotQuat = xform.rotQuat;
			activeShield_sp->xform.position = xform.position;
		}
	}

	void Ship::startCarrierExplosionSequence()
	{
		bool bFailure = true;

		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
			{
				bFailure = false; //at this point no need to flag failure, we've either started or are about to start the destroy sequence

				//if we have a timer delegate, then we've already ran this code and no need to start another one (thinking this can happen fi ship is shot during explosion)
				if (!carrierDestroyTickTimerDelegate)
				{
					carrierDestroyTickTimerDelegate = new_sp<MultiDelegate<>>();
					carrierDestroyTickTimerDelegate->addWeakObj(sp_this(), &Ship::timerTick_CarrierExplosion);

					float timerDuration = 0.5f;
					worldTimeManager->createTimer(carrierDestroyTickTimerDelegate, timerDuration, /*loop*/ true);
				}
			}
		}

		if(bFailure)
		{
			destroy();
		}
	}

	void Ship::timerTick_CarrierExplosion()
	{
		constexpr size_t MAX_CARRIER_EXPLOISION_TICKS = 20;

		++numExplosionSequenceTicks;
		if (numExplosionSequenceTicks > MAX_CARRIER_EXPLOISION_TICKS)
		{
			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
			const sp<TimeManager>& worldTimeManager = currentLevel ? currentLevel->getWorldTimeManager() : nullptr;
			if (carrierDestroyTickTimerDelegate && worldTimeManager)
			{
				worldTimeManager->removeTimer(carrierDestroyTickTimerDelegate); //don't loop any more
			}
			else{STOP_DEBUGGER_HERE();}

			destroy(); //finally, destroy this for real 
		}
		else
		{
			if (rng)
			{
				size_t placementArrayIndex = rng->getInt<size_t>(0, 2);
				sp<ShipPlacementEntity> selectedObjectiveToSpawnExplosionAt = nullptr;

				////////////////////////////////////////////////////////
				// select a random object to spawn the explosion at
				////////////////////////////////////////////////////////
				auto selectLambda = [this](std::vector<sp<ShipPlacementEntity>> placementArray) {
					if (placementArray.size() > 0)
					{
						return placementArray[rng->getInt<size_t>(0, placementArray.size() - 1)];
					}
					else{return sp<ShipPlacementEntity>(nullptr);}
				};
				switch (placementArrayIndex)
				{
					case 0:	selectedObjectiveToSpawnExplosionAt = selectLambda(generatorEntities);
						break;
					case 1:	selectedObjectiveToSpawnExplosionAt = selectLambda(turretEntities);
						break;
					case 2:	selectedObjectiveToSpawnExplosionAt = selectLambda(communicationEntities);
						break;
					default:
						break;
				}

				////////////////////////////////////////////////////////
				// use the selected objective to spwan the explosion
				////////////////////////////////////////////////////////
				if (selectedObjectiveToSpawnExplosionAt)
				{
					glm::vec3 selectedObjectivePosition = selectedObjectiveToSpawnExplosionAt->getWorldPosition();

					//VFX
					float explosionScaleUp = 20.f;

					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
					particleSpawnParams.xform.position = selectedObjectivePosition;
					particleSpawnParams.xform.scale = glm::vec3(explosionScaleUp);
					particleSpawnParams.velocity = getVelocity();

					//SFX
					GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
					if (sp<AudioEmitter> sfx_explosionEmitter = shipConfigData ? GameBase::get().getAudioSystem().createEmitter() : nullptr)
					{
						shipConfigData->getConfig_sfx_explosion().configureEmitter(sfx_explosionEmitter);
						sfx_explosionEmitter->setPriority(AudioEmitterPriority::GAMEPLAY_CRITICAL);
						sfx_explosionEmitter->setPosition(selectedObjectivePosition);
						sfx_explosionEmitter->setVelocity(getVelocity());
						sfx_explosionEmitter->play();
						carrierExplosionSFX.push_back(sfx_explosionEmitter);
					}
				}
				else
				{
					STOP_DEBUGGER_HERE(); //there's no selected placement entity? how did this happen?
					return;
				}


			}
			else { STOP_DEBUGGER_HERE(); }
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
		if (bEnableAvoidanceFields && collisionData && !bNoVelocity )//&& !isCarrierShip())
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

						avoidStrength *= avoidanceSensitivity; //some use cases (eg targeting player) require depended avoidance with known cost of collision

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

								//if this line below is flashing, it is likely we're generating zero vectors (not yet seen, but consciously putting this in branch so that can be indicated)
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

	void Ship::regenerateEngineVFX()
	{
		destroyEngineVFX();

		const std::vector<EngineEffectData>& fxData = shipConfigData->getEngineEffectData();
		for (const EngineEffectData& fx : fxData)
		{
			const Transform& shipXform = this->getTransform();
			sp<EngineParticleEffectConfig> particleCFG = nullptr;
			
			if (fx.bOverrideColorWithTeamColor)
			{
				EngineEffectData fxColorized = fx;
				//fxColorized.color = cachedTeamData.shieldColor;
				fxColorized.color = cachedTeamData.projectileColor;
				particleCFG = SharedGFX::get().engineParticleCache->getParticle(fxColorized);
			}
			else
			{
				particleCFG = SharedGFX::get().engineParticleCache->getParticle(fx);
			}

			ParticleSystem::SpawnParams particleSpawnParams;
			particleSpawnParams.particle = particleCFG;
			particleSpawnParams.xform.position = shipXform.position;
			particleSpawnParams.xform.rotQuat = shipXform.rotQuat;
			particleSpawnParams.xform.scale = shipXform.scale;

			//#TODO #REFACTOR hacky as only considering scale. particle perhaps should use matrices to avoid this, or have list of transform to apply.
			//making the large ships show correct effect. Perhaps not even necessary.
			Transform modelXform = shipConfigData->getModelXform();
			particleSpawnParams.xform.scale *= modelXform.scale;

			wp<ActiveParticleGroup> weakParticle = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
			if (sp<ActiveParticleGroup> engineParticle = weakParticle.lock())
			{
				engineFireParticlesFX.push_back(engineParticle);
			}
		}

		//make sure we correctly position engine fx
		tickEngineFX();
	}

	void ShipUtilLibrary::setEngineParticleOffset(Transform& outParticleXform, const Transform& shipXform, const EngineEffectData& effectData)
	{
		glm::vec3 shipWorldPos = shipXform.position; //NOTE this may be different than xform is ship has a parent, but this is not planned right now #todo #scenenodes

		//Transform shipConfigXform = shipConfigData->getModelXform();
		outParticleXform.scale = effectData.localScale; //*shipConfigXform.scale; //#TODO #HACK using shipconfig xform to change scale for large ships
		outParticleXform.position = shipXform.rotQuat*(effectData.localOffset /** shipConfigXform.scale*/) + shipWorldPos;
	}

	void Ship::tickEngineFX()
	{
		glm::vec3 shipWorldPos = getWorldPosition();
		const Transform& shipXform = getTransform();

		const std::vector<EngineEffectData>& engineEffectData = shipConfigData->getEngineEffectData();
		for (size_t engineFxIdx = 0; engineFxIdx < engineEffectData.size() && engineFxIdx < engineFireParticlesFX.size(); ++engineFxIdx)
		{
			const EngineEffectData& effectData = engineEffectData[engineFxIdx];
			const sp<ActiveParticleGroup>& engineParticle = engineFireParticlesFX[engineFxIdx];
			if (engineParticle)
			{
				ShipUtilLibrary::setEngineParticleOffset(engineParticle->xform, shipXform, effectData);

				//incrase engine size based on boost
				if (bool bEnableBooseInfluenceOnEngineFX = true)
				{
					engineParticle->xform.scale *= adjustedBoost;
				}
			}else{ STOP_DEBUGGER_HERE(); /*what happened, we lost a particle O.o*/}
		}
	}

	void Ship::destroyEngineVFX()
	{
		for (const sp<ActiveParticleGroup>& engineParticle : engineFireParticlesFX)
		{
			if (engineParticle)
			{
				engineParticle->killParticle();
			}
		}
		engineFireParticlesFX.clear();
	}

	void Ship::configureForEditorMode(const SpawnData& spawnData)
	{
		shipConfigData = spawnData.spawnConfig;

		if (TeamComponent* teamComp = !hasGameComponent<TeamComponent>() ? createGameComponent<TeamComponent>() : getGameComponent<TeamComponent>())
		{
			teamComp->setTeam(cachedTeamIdx);
			updateTeamDataCache();
		}
	}

	void Ship::handlePlacementDestroyed(const sp<GameEntity>& entity)
	{
		//unbind from this event so decrementing can never happen twice
		entity->onDestroyedEvent->removeAll(sp_this());

		if (ShipPlacementEntity* placement = dynamic_cast<ShipPlacementEntity*>(entity.get())) //this cast should be safe -- but doing dynamic for code quality reasons and future proofing; this will not happen a lot
		{
			if (placement->getPlacementType() == PlacementType::DEFENSE)
			{
				if (--activeGenerators == 0.f)
				{
					for (const sp<ShipPlacementEntity>& commPlacement : communicationEntities) { if (commPlacement) commPlacement->setHasGeneratorPower(false); }
					for (const sp<ShipPlacementEntity>& turretPlacement: turretEntities) { if (turretPlacement) turretPlacement->setHasGeneratorPower(false); }
				}
			}
			else if (placement->getPlacementType() == PlacementType::TURRET)
			{
				--activeTurrets;
			}
			else if (placement->getPlacementType() == PlacementType::COMMUNICATIONS)
			{
				--activeCommunications;
			}

		}

		activePlacements -= 1;
	}

	void Ship::handleSpawnStasisOver()
	{
		BrainComponent* brainComp = getGameComponent<BrainComponent>();
		if (brainComp && brainComp->brain && bAwakeBrainAfterStasis)
		{
			brainComp->brain->awaken();
		}
	}

	void Ship::handleSpawnedEntity(const sp<Ship>& ship)
	{
		fwp<FighterSpawnComponent> weakSpawnComp = fighterSpawnComp->requestTypedReference_Safe<std::decay<decltype(*fighterSpawnComp)>::type>();
		ship->setOwningSpawnComponent(weakSpawnComp);
	}

	void Ship::handleHpAdjusted(const HitPoints& old, const HitPoints& hp)
	{
		if (bool bDestroyed = hp.current <= 0.f)
		{
			if (!isPendingDestroy())
			{
				if (!isCarrierShip()) //is fighter shp
				{
					if (!activeShieldEffect.expired())
					{
						sp<ActiveParticleGroup> shieldVFX = activeShieldEffect.lock();
						shieldVFX->killParticle();
						shieldVFX->xform.scale = glm::vec3(0.f); //particle isn't disappearing... 
						activeShieldEffect.reset();
					}

					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
					particleSpawnParams.xform.position = this->getTransform().position;
					particleSpawnParams.velocity = getVelocity();

					GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
					if (sfx_explosion) 
					{ 
						sfx_explosion->setPosition(getWorldPosition());
						sfx_explosion->setVelocity(getVelocity());
						sfx_explosion->play(); 
					}
					if (sfx_engine) { sfx_engine->stop(); }

					if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
					{
						brainComp->setNewBrain(sp<AIBrain>(nullptr));
					}

					if (transientCollidingProjectile && transientCollidingProjectile->owner)
					{ //if you get a crash here, did we clean this up after being hit? this should always be cleaned up within the scope that the player is hit by a projectile!
						if (OwningPlayerComponent* playerComp = transientCollidingProjectile->owner->getGameComponent<OwningPlayerComponent>())
						{
							if (playerComp->hasOwningPlayer())
							{
								//#todod #nextengine indexed dynamic casts with class header injected code? use index created at runtime to identify classes or something, this should let us use an index and then static cast if index is in an array of classes that are available or something (may need to tweak)
								sp<PlayerBase> playerBase = playerComp->getOwningPlayer().lock();
								if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerBase.get()))
								{
									player->incrementKillCount(); //this could be virtual to avoid dynamic cast. but may be weird to put this in framework base. 
								}
							}
						}
					}

				}
				else
				{
					startCarrierExplosionSequence();
				}
			}

			if (!isCarrierShip()) //carrier ship will have special destroy sequence
			{
				destroy(); //perhaps enter a destroyed state with timer to remove actually destroy -- rather than immediately despawning
			}
		}
		else
		{
			doShieldFX();
		}
	}

#if COMPILE_CHEATS
	void Ship::cheat_oneShotPlacements()
	{
		auto cheatLambda = [this](std::vector<sp<ShipPlacementEntity>> placements)
		{
			for (sp<ShipPlacementEntity>& placement : placements)
			{
				if (HitPointComponent* hpComp = placement->getGameComponent<HitPointComponent>())
				{
					//#suggested perhaps cleaner to adjust it on the component and the entity will react the adjustments?
					bool cachedPower = placement->hasGeneratorPower();
					placement->setHasGeneratorPower(false); //don't let generator shield stop this
					placement->adjustHP(-hpComp->getHP().current + 1.f);
					placement->setHasGeneratorPower(cachedPower); //restore power if they had it.
				}
			}
		};
		cheatLambda(generatorEntities);
		cheatLambda(communicationEntities);
		cheatLambda(turretEntities);
	}

	void Ship::cheat_destroyAllShipPlacements()
	{
		auto cheatLambda = [this](std::vector<sp<ShipPlacementEntity>> placements)
		{
			for (sp<ShipPlacementEntity>& placement : placements)
			{
				if (HitPointComponent* hpComp = placement->getGameComponent<HitPointComponent>())
				{
					//#suggested perhaps cleaner to adjust it on the component and the entity will react the adjustments?
					placement->setHasGeneratorPower(false);//this will prevent it from destroying as generator power adds sheild that reduces damage
					placement->adjustHP(-hpComp->getHP().current);
				}
			}
		};
		cheatLambda(generatorEntities);
		cheatLambda(communicationEntities);
		cheatLambda(turretEntities);
	}

	void Ship::cheat_destroyAllGenerators()
	{
		for (sp<ShipPlacementEntity>& generator : generatorEntities)
		{
			if (HitPointComponent* hpComp = generator->getGameComponent<HitPointComponent>())
			{
				//#suggested perhaps cleaner to adjust it on the component and the entity will react the adjustments?
				generator->setHasGeneratorPower(false);
				generator->adjustHP(-hpComp->getHP().current);
			}
		}
	}

	void Ship::cheat_turretsTargetPlayer()
	{
		using TargetType = ShipPlacementEntity::TargetType;

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

	void Ship::cheat_commsTargetPlayer()
	{
		using TargetType = ShipPlacementEntity::TargetType;
		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (TargetType* playerShip = dynamic_cast<TargetType*>(player->getControlTarget()))
			{
				wp<TargetType> wpPlayerShip = playerShip->requestTypedReference_Nonsafe<TargetType>();
				for (const sp<ShipPlacementEntity>& commBase : communicationEntities)
				{
					if (CommunicationPlacement* satellite = dynamic_cast<CommunicationPlacement*>(commBase.get()))
					{
						satellite->setTarget(wpPlayerShip);
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
		assert(shipConfigData);
		if (shipConfigData && generatorEntities.size() == 0 && communicationEntities.size() == 0 && turretEntities.size() == 0)
		{
			ShipPlacementEntity::TeamData teamData;
			teamData.team = cachedTeamIdx;

			const std::vector<TeamData>& teams = shipConfigData->getTeams();
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
						newPlacement->replacePlacementConfig(placementConfig, *shipConfigData);
						newPlacement->setTeamData(teamData);
						newPlacement->setHasGeneratorPower(true);
						outputContainer.push_back(newPlacement);
					}
				}
			};

			processPlacements(shipConfigData->getDefensePlacements(), generatorEntities);
			processPlacements(shipConfigData->getCommuncationPlacements(), communicationEntities);
			processPlacements(shipConfigData->getTurretPlacements(), turretEntities);

			activePlacements = generatorEntities.size() + communicationEntities.size() + turretEntities.size();
		}
	}

	void Ship::postctor_configureObjectivePlacements()
	{
		sp<Ship> this_sp = sp_this();

		auto processPlacements = [&](const std::vector<sp<ShipPlacementEntity>>& container)
		{
			for (const sp<ShipPlacementEntity>& placement : container)
			{
				if (placement)
				{
					placement->setWeakOwner(this_sp);
				}
			}
		};

		processPlacements(generatorEntities);
		processPlacements(communicationEntities);
		processPlacements(turretEntities);
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


	void ShipUtilLibrary::setShipTarget(const sp<Ship>& ship, const lp<WorldEntity>& target)
	{
		if (BrainComponent* brainComp = ship ? ship->getGameComponent<BrainComponent>() : nullptr)
		{
			////////////////////////////////////////////////////////
			// target objective
			////////////////////////////////////////////////////////
			if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
			{
				using namespace BehaviorTree;
				Memory& memory = behaviorTree->getMemory();
				{
					sp<WorldEntity> targetSP = target;
					memory.replaceValue(BT_TargetKey, targetSP);
				}
			}
		}
	}



}
