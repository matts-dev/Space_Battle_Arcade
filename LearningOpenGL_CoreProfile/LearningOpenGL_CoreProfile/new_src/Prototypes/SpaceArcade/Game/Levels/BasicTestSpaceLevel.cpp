#include "BasicTestSpaceLevel.h"

#include <random>
#include <memory>
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"

#include "../SpaceArcade.h"
#include "../SAProjectileSystem.h"
#include "../SAModSystem.h"
#include "../SAUISystem.h"
#include "../SAShip.h"
#include "../AssetConfigs/SASpawnConfig.h"
#include "../Tools/Debug/SAHitboxPicker.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../GameFramework/RenderModelEntity.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SALog.h"
#include "../../Rendering/Camera/SACameraBase.h"

#include "../SACollisionDebugRenderer.h"
#include "../UI/SAProjectileTweakerWidget.h"
#include "../AI/SAShipAIBrain.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../GameFramework/SAParticleSystem.h"
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "../../GameFramework/SAAIBrainBase.h"
#include "../../GameFramework/SABehaviorTree.h"
#include "../AI/SADogfightNodes_LargeTree.h"
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/Interfaces/SAIControllable.h"
#include "../Environment/StarField.h"
#include "../Environment/Planet.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../Environment/Star.h"
#include "../../GameFramework/Components/CollisionComponent.h"
#include "../../Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"
#include "../Cameras/SAShipCamera.h"

namespace SA
{
	void BasicTestSpaceLevel::postConstruct()
	{
		SpaceLevelBase::postConstruct();

		ambientLight = glm::vec3(0.01f);

		SpaceArcade& game = SpaceArcade::get();

		if (const sp<ModSystem>& modSystem = game.getModSystem())
		{
			modSystem->onActiveModChanging.addWeakObj(sp_this(), &BasicTestSpaceLevel::handleActiveModChanging);
		}

		projectileWidget = new_sp<ProjectileTweakerWidget>();
		hitboxPickerWidget = new_sp<HitboxPicker>();

		testParticleConfig = ParticleFactory::getSimpleExplosionEffect();
		testParticles.insert({ "simple explosion", testParticleConfig});
	}

	void BasicTestSpaceLevel::startLevel_v()
	{
#if ERROR_CHECK_GL_RELEASE 
	log("BasicTestSpaceLevel", LogLevel::LOG_WARNING, "ERROR_CHECK_GL_RELEASE enabled. For performance this should be disabled for final release builds.");
#endif

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
		if (!activeMod)
		{
			log("BasicTestSpaceLevel", LogLevel::LOG_ERROR, "No active mod");
			return;
		}

		const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
		if (const auto& iter = spawnConfigs.find("Fighter"); iter != spawnConfigs.end())
		{
			fighterSpawnConfig = iter->second;
		}
		if (const auto& iter = spawnConfigs.find("Carrier"); iter != spawnConfigs.end())
		{
			carrierSpawnConfig = iter->second;
		}

		if (!fighterSpawnConfig)
		{
			log("BasicTestSpaceLevel", LogLevel::LOG_ERROR, "Default Spawn Configs not available.");
		}

		glm::vec3 carrierPosition_teamA = { 0,0, 150 };
		//glm::vec3 carrierPosition_teamA = { 0,0, 30 }; //testing dogfight

		if (const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			const sp<CameraBase>& camera = player->getCamera();
			camera->setFar(1000.f);

			//camera->setPosition(glm::vec3(-300, 0, 0.f));
			camera->setPosition(carrierPosition_teamA + glm::vec3(15, 20, 20));
			camera->lookAt_v(camera->getPosition() + glm::vec3(0,0,-1)); //carriers are currently separated along the z axis; so look down that axis
		}

		////////////////////////////////////////////////////////
		// Carriers
		////////////////////////////////////////////////////////
		constexpr bool bSpawnCarriers = true;
		Transform carrierXform_TeamA;
		carrierXform_TeamA.position = carrierPosition_teamA;
		carrierXform_TeamA.rotQuat = glm::angleAxis(glm::radians(-33.0f), glm::vec3(0, 1, 0));

		Ship::SpawnData carrierSpawnData_A;
		carrierSpawnData_A.team = 0;
		carrierSpawnData_A.spawnConfig = carrierSpawnConfig;
		carrierSpawnData_A.spawnTransform = carrierXform_TeamA;
		sp<Ship> carrierShip_TeamA = bSpawnCarriers ? spawnEntity<Ship>(carrierSpawnData_A) : nullptr;

		//#TODO #BUG passing same carrier spawn data shouldn't influence the other ships that were spawned. copying spawn data.
		Ship::SpawnData carrierSpawnData_B = carrierSpawnData_A;
		Transform carrierXform_TeamB = carrierXform_TeamA;
		carrierXform_TeamB.position.z = -carrierXform_TeamB.position.z;
		carrierXform_TeamB.rotQuat = glm::angleAxis(glm::radians(-13.0f), glm::vec3(0, 1, 0));
		carrierSpawnData_B.spawnTransform = carrierXform_TeamB;
		carrierSpawnData_B.team = 1;
		//sp<Ship> carrierShip2 = spawnEntity<Ship>(carrierModel, carrierXform_TeamB, createUnitCubeCollisionInfo());
		sp<Ship> carrierShip_TeamB = bSpawnCarriers ? spawnEntity<Ship>(carrierSpawnData_B) : nullptr;

		////////////////////////////////////////////////////////
		// fighters
		////////////////////////////////////////////////////////
		particleSpawnOffset = glm::vec3(0,0,0);

		std::random_device rng;
		std::seed_seq seed{ 28 };
		std::mt19937 rng_eng = std::mt19937(seed);
		std::uniform_real_distribution<float> startDist(-50.f, 50.f); //[a, b)

		uint32_t numFighterShipsToSpawn = 5000;
#ifdef _DEBUG
		//numFighterShipsToSpawn = 250;
		//numFighterShipsToSpawn = 50;
		//numFighterShipsToSpawn = 20;
		//numFighterShipsToSpawn = 10;
		//numFighterShipsToSpawn = 4;
		numFighterShipsToSpawn = 2;
#endif//NDEBUG 

		uint32_t numTeams = 2;
		uint32_t numFightersPerTeam = numFighterShipsToSpawn / numTeams;

		std::vector< std::vector<sp<Ship>> > teamTargets = {
			std::vector<sp<Ship>>{},
			std::vector<sp<Ship>>{}
		};

		auto spawnFighters = [&](size_t teamIdx, glm::vec3 teamSpawnOrigin) 
		{
			Ship::SpawnData fighterShipSpawnData;
			fighterShipSpawnData.team = teamIdx;
			fighterShipSpawnData.spawnConfig = fighterSpawnConfig;

			for (uint32_t fighterShip = 0; fighterShip < numFightersPerTeam; ++fighterShip)
			{ 
				glm::vec3 startPos(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));
				glm::quat rot = glm::angleAxis(startDist(rng_eng), glm::vec3(0, 1, 0)); //angle is a little adhoc, but with radians it should cover full 360 possibilities
				startPos += teamSpawnOrigin;

				fighterShipSpawnData.spawnTransform = Transform{ startPos, rot, {0.1,0.1,0.1} };

				sp<Ship> fighter = spawnEntity<Ship>(fighterShipSpawnData);
				teamTargets[teamIdx].push_back(fighter);

				//fighter->spawnNewBrain<FlyInDirectionBrain>(); 
				//fighter->spawnNewBrain<DogfightTestBrain_VerboseTree>();
				fighter->spawnNewBrain<WanderBrain>();
				//fighter->spawnNewBrain<EvadeTestBrain>();
				//fighter->spawnNewBrain<DogfightTestBrain>();

				//fighter->spawnNewBrain<FighterBrain>();
			}
		};
		spawnFighters(0, carrierXform_TeamA.position);
		spawnFighters(1, carrierXform_TeamB.position);

		if(const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			player->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_LEFT).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleLeftMouseButton);
			player->getInput().getMouseButtonEvent(GLFW_MOUSE_BUTTON_RIGHT).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleRightMouseButton);
			player->getInput().getKeyEvent(GLFW_KEY_P).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleDebugCameraRequested);
			player->getInput().getKeyEvent(GLFW_KEY_X).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleSpectateDetachPressed);
			player->getInput().getKeyEvent(GLFW_KEY_C).addWeakObj(sp_this(), &BasicTestSpaceLevel::handlePlayerControlTarget);
			player->getInput().getKeyEvent(GLFW_KEY_G).addWeakObj(sp_this(), &BasicTestSpaceLevel::handleNavigateStarfieldPressed);
		}

		//pick a projectile to test with
		if (const sp<Mod>& activeMod = game.getModSystem()->getActiveMod())
		{
			int idx = 0;
			for (auto projectileConfigIter : activeMod->getProjectileConfigs())
			{
				if (idx == selectedProjectileConfigIdx)
				{
					testProjectileConfig = projectileConfigIter.second;
					break;
				}
				idx++;
			}
		}

		//DEBUG assign targets to each other to test dogfighting
		bool bEnableDebugTargets = false;
		if(bEnableDebugTargets)
		{
			size_t targetsToSet = teamTargets[0].size() < teamTargets[1].size() ? teamTargets[0].size() : teamTargets[1].size();
			for (size_t idx = 0; idx < targetsToSet ; idx++)
			{
				sp<Ship> a = teamTargets[0][idx];
				sp<Ship> b = teamTargets[1][idx];

				const BrainComponent* aBrainComp = a->getGameComponent<BrainComponent>();
				const BrainComponent* bBrainComp = b->getGameComponent<BrainComponent>();
				if (aBrainComp && bBrainComp)
				{
					BehaviorTreeBrain* aBrain = dynamic_cast<BehaviorTreeBrain*>(aBrainComp->getBrain());
					BehaviorTreeBrain* bBrain = dynamic_cast<BehaviorTreeBrain*>(bBrainComp->getBrain());
					if (aBrain && bBrain)
					{
						BehaviorTree::Memory& aMem = aBrain->getBehaviorTree().getMemory();
						BehaviorTree::Memory& bMem = bBrain->getBehaviorTree().getMemory();

						sp<WorldEntity> aWE = a;
						sp<WorldEntity> bWE = b;

						WorldEntity* bAsTarget = aMem.replaceValue("target", bWE);
						WorldEntity* aAsTarget = bMem.replaceValue("target", aWE);
					}
				}
			}
		}


		//follow a target from the start
		size_t targetIdx = 0;
		size_t pickIdx = 0;
		for (sp<WorldEntity> entity : worldEntities)
		{
			if (pickIdx == targetIdx)
			{
				if (Ship* ship = dynamic_cast<Ship*>(entity.get()))
				{
					if (!ship->hasAvoidanceSpheres())
					{
						hitboxPickerWidget->setPickTarget(entity);
						break;
					}
					else
					{
						//try the next one as this one is a large ship with avoidance attached
						targetIdx++;
					}
				}
			}
			pickIdx++;
		}
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
					if (!camera->isInCursorMode() && !player->hasControlTarget())
					{
						if (testProjectileConfig)
						{
							const sp<ProjectileSystem>& projectileSys = game.getProjectileSystem();

							ProjectileSystem::SpawnData spawnData;
							spawnData.start = camera->getPosition() + glm::vec3(0, -0.25f, 0);
							spawnData.direction_n = camera->getFront();
							spawnData.team = -1; //#TODO get player's ship and get team
							spawnData.damage = 25;

							projectileSys->spawnProjectile(spawnData, *testProjectileConfig);
						}

						if (bFreezeTimeOnClick_ui)
						{
							getWorldTimeManager()->setTimeFreeze(true);
						}
					}
				}
			}
		}
	}

	void BasicTestSpaceLevel::handleRightMouseButton(int state, int modifier_keys)
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
						const sp<TimeManager>& worldTime = getWorldTimeManager();
						if (worldTime->isTimeFrozen())
						{
							getWorldTimeManager()->setFramesToStep(1);
						}
					}
				}
			}
		}
	}

	void BasicTestSpaceLevel::handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active)
	{
		std::random_device rng;
		std::seed_seq seed{ 28 };
		std::mt19937 rng_eng = std::mt19937(seed);
		std::uniform_real_distribution<float> startDist(-200.f, 200.f); //[a, b)
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
		if (bInCursorMode) 
		{ 
			const sp<TimeManager>& timeManager = getWorldTimeManager();

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
#if SA_RENDER_DEBUG_INFO
				ImGui::Checkbox("Render entity OBB pretests", &CollisionDebugRenderer::bRenderCollisionOBB_ui);
				ImGui::Checkbox("Render entity collision shapes", &CollisionDebugRenderer::bRenderCollisionShapes_ui);
#endif //SA_RENDER_DEBUG_INFO
#if SA_CAPTURE_SPATIAL_HASH_CELLS
				ImGui::Checkbox("Render Spatial Hash Cells", &game.bRenderDebugCells);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
				if constexpr (bCompileDebugDebugSpatialHashVisualizations)
				{
					ImGui::Checkbox("Render Avoidance Spatial Hash Cells", &AvoidanceSphere::bDebugSpatialHashVisualization);
				}
				if constexpr (bDebugAvoidance)
				{
					ImGui::Checkbox("Render ship avoidance widgets", &SA::bDrawAvoidance_debug);
				}
				if (ImGui::Checkbox("force player to experience AI avoidance (requires controling new target)", &SA::bForcePlayerAvoidance_debug))
				{
					//GameBase::get().getPlayerSystem().getPlayer(0)->setControlTarget() //perhaps make method to get control target and have it refresh
				}

				static bool bRenderAvoidanceSphereProxy = false;
				if(ImGui::Checkbox("Render Avoidance Spheres", &bRenderAvoidanceSphereProxy)){Ship::setRenderAvoidanceSpheres(bRenderAvoidanceSphereProxy);}
				ImGui::Checkbox("Render Projectile OBBs", &game.bRenderProjectileOBBs);

				////////////////////////////////////////////////////////
				// TIME 
				////////////////////////////////////////////////////////
				ImGui::Dummy(ImVec2(0, 20.f));
				ImGui::Separator();
				if (ImGui::Button("ToggleTimeFreeze")) 
				{ 
					timeManager->setTimeFreeze(!timeManager->isTimeFrozen());
				}
				if (ImGui::Button("Step Time 1 Frame"))
				{
					timeManager->setFramesToStep(1);
				}
				ImGui::Checkbox("Freeze Time On Click", &bFreezeTimeOnClick_ui);
				if (ImGui::SliderFloat("Time Dilation", &timeDilationFactor_ui, 0.001f, 4.f))
				{
					timeManager->setTimeDilationFactor_OnNextFrame(timeDilationFactor_ui);
				}
				if (ImGui::Button("reset dilation"))
				{
					timeDilationFactor_ui = 1.f;
					timeManager->setTimeDilationFactor_OnNextFrame(timeDilationFactor_ui);
				}

				////////////////////////////////////////////////////////
				// Projectiles Optimization
				////////////////////////////////////////////////////////
				ImGui::Dummy(ImVec2(0, 20.f));
				ImGui::Separator();
				if (ImGui::Checkbox("Enable All Ships Continuous Fire", &bForceShipsToFire_ui))
				{
					refreshShipContinuousFireState();
				}
				if (ImGui::InputFloat("Ship Fire Rate Seconds", &forceFireRateSecs_ui))
				{
					bForceShipsToFire_ui = false;
					refreshShipContinuousFireState();
				}

				////////////////////////////////////////////////////////
				// Projectile Tweaker
				////////////////////////////////////////////////////////
				ImGui::Dummy(ImVec2(0, 20.f));
				ImGui::Separator();
				ImGui::Checkbox("Show Projectile Tweaker", &bShowProjectileTweaker_ui);
				if (ImGui::CollapsingHeader("Test Projectile", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
					if (!activeMod)
					{
						ImGui::Text("No active mode");

					}
					else
					{
						const std::map<std::string, sp<ProjectileConfig>>& projectileConfigs = activeMod->getProjectileConfigs();
						int curConfigIdx = 0;
						for (const auto& kvPair : projectileConfigs)
						{
							if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedProjectileConfigIdx))
							{
								testProjectileConfig = kvPair.second;
								selectedProjectileConfigIdx = curConfigIdx;
								refreshProjectiles();
							}
							++curConfigIdx;
						}
					}
				}

				////////////////////////////////////////////////////////
				// Particle Tests
				////////////////////////////////////////////////////////
				ImGui::Dummy(ImVec2(0, 20.f));
				ImGui::Separator();

				if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Particle Spawning");
					{
						int curConfigIdx = 0;
						for (const auto& kvPair : testParticles)
						{
							if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedTestParticleIdx))
							{
								testParticleConfig = kvPair.second;
								selectedTestParticleIdx = curConfigIdx;
							}
							++curConfigIdx;
						}
					}
					ImGui::Checkbox("Continuous Spawn", &bContinuousSpawns_ui); //TODO have this check that the particle isn't looping
					if (ImGui::Button("Start/Stop spawn"))
					{
						toggleSpawnTestParticles();
					}
					ImGui::InputFloat2("spawn bounds", &particleSpawnBounds.x);
					ImGui::InputFloat3("spawn offset", &particleSpawnOffset.x);
					ImGui::SliderInt("#Particle Init Spawns", &numParticleSpawns_ui, 1, 10);
					ImGui::SliderInt("Spawn Multiplier", &numSpawnMultiplier_ui, 1, 1000);
					ImGui::SliderInt("#Batches", &numSpawnBatches, 1, 10);

				}
			}
			ImGui::End();
		}

		////////////////////////////////////////////////////////
		// Windows persistent without cursor mode
		////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////
		// Projectile Tweaker
		////////////////////////////////////////////////////////
		if (bShowProjectileTweaker_ui)
		{
			ImGuiWindowFlags projWindowFlags = 0;
			//flags |= ImGuiWindowFlags_NoMove;
			//flags |= ImGuiWindowFlags_NoResize;
			projWindowFlags |= ImGuiWindowFlags_NoCollapse;
			ImGui::Begin("Projectile Tweaker!", nullptr, projWindowFlags);
				projectileWidget->renderInCurrentUIWindow();
			ImGui::End();

		}
	}

	void BasicTestSpaceLevel::handleEntityDestroyed(const sp<GameEntity>& entity)
	{
		if (sp<Ship> ship = std::dynamic_pointer_cast<Ship>(entity))
		{
			unspawnEntity<Ship>(ship);
		}
	}

	void BasicTestSpaceLevel::handleDebugCameraRequested(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_RELEASE)
		{
			const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0);
			const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
			if (player && primaryWindow)
			{
				sp<CameraBase> newCamera = new_sp<QuaternionCamera>();
				newCamera->registerToWindowCallbacks_v(primaryWindow);
				player->setCamera(newCamera);
			}
		}
	}

	void BasicTestSpaceLevel::handleSpectateDetachPressed(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_RELEASE)
		{
			hitboxPickerWidget->setPickTarget(sp<WorldEntity>(nullptr));

			const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0);
			player->setControlTarget(sp<IControllable>(nullptr));

			if (ShipCamera* shipCamera = dynamic_cast<ShipCamera*>(player->getCamera().get()))
			{
				//only set up new camera if player has started controlling a ship.
				//set up a new camera so that the ship camera is cleared from player.
				const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
				sp<CameraBase> newCamera = new_sp<QuaternionCamera>();
				newCamera->registerToWindowCallbacks_v(primaryWindow);
				player->setCamera(newCamera);
			}
		}
	}

	void BasicTestSpaceLevel::handlePlayerControlTarget(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_RELEASE && !(modifier_keys & GLFW_MOD_ALT))
		{
			if (hitboxPickerWidget)
			{
				sp<WorldEntity> pickTarget = hitboxPickerWidget->getPickTarget();
				if (sp<IControllable> controllable = std::dynamic_pointer_cast<IControllable>(pickTarget))
				{
					GameBase::get().getPlayerSystem().getPlayer(0)->setControlTarget(controllable);
				}
			}
		}
	}

	void BasicTestSpaceLevel::handleNavigateStarfieldPressed(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			//make the environment on a similar scale as the ships for debugging.
			bool bNavigable = !starField->getForceCentered();
			starField->setForceCentered(bNavigable);
			for (const sp<Star>& star : localStars)
			{
				star->setForceCentered(bNavigable);
			}

			for (const sp<Planet>& planet : planets)
			{
				planet->setForceCentered(bNavigable); //#TODO probably a good idea at this point to create an "CelestialEnvironmentObj" base class for the shared functionality
			}
		}
	}

	void BasicTestSpaceLevel::refreshShipContinuousFireState()
	{
		std::random_device rng;
		std::seed_seq seed{ 7 };
		std::mt19937 rng_eng = std::mt19937(seed);
		std::uniform_real_distribution<float> fireDelayDistribution(0.f, forceFireRateSecs_ui); //[a, b)

		for (const sp<Ship>& ship : spawnedShips)
		{
			if (BrainComponent* brainComp = ship->getGameComponent<BrainComponent>())
			{
				if (bForceShipsToFire_ui)
				{
					sp<ContinuousFireBrain> cfBrain = new_sp<ContinuousFireBrain>(ship);
					cfBrain->setDelayStartFire(fireDelayDistribution(rng_eng));
					cfBrain->setFireRateSecs(forceFireRateSecs_ui);
					brainComp->setNewBrain(cfBrain);
				}
				else
				{
					sp<FlyInDirectionBrain> singleDirectionBrain = new_sp<FlyInDirectionBrain>(ship);
					brainComp->setNewBrain(singleDirectionBrain);
				}

			}
		}
	}

	void BasicTestSpaceLevel::refreshProjectiles()
	{
		if (testProjectileConfig)
		{
			for (const sp<Ship>& ship : spawnedShips)
			{
				ship->setPrimaryProjectile(testProjectileConfig);
			}
		}
	}

	void BasicTestSpaceLevel::toggleSpawnTestParticles()
	{
		bSpawningParticles = !bSpawningParticles;

		const sp<TimeManager>& worldTM = getWorldTimeManager();
		if (bSpawningParticles)
		{
			if (testParticleConfig)
			{
				//start spawning particles
				if (bContinuousSpawns_ui)
				{
					float spawnInterval = testParticleConfig->getDurationSecs() / numSpawnBatches;
					numParticlesInBatch = (numParticleSpawns_ui*numSpawnMultiplier_ui) / numSpawnBatches;
					if (spawnInterval <= 0 || numParticlesInBatch  <= 0) { return; }

					particleSpawnDelegate = new_sp<MultiDelegate<>>();
					particleSpawnDelegate->addWeakObj(sp_this(), &BasicTestSpaceLevel::handleTestParticleSpawn);
					worldTM->createTimer(particleSpawnDelegate, spawnInterval, true);

					handleTestParticleSpawn();
				}
				else
				{
					numParticlesInBatch = static_cast<int>(numParticleSpawns_ui * numSpawnMultiplier_ui);
					handleTestParticleSpawn();

					//not continuous, so don't change state
					bSpawningParticles = false;
				}
			}
		}
		else
		{
			//stop spawning particles
			if (particleSpawnDelegate && worldTM->hasTimerForDelegate(particleSpawnDelegate))
			{
				worldTM->removeTimer(particleSpawnDelegate);
			}
		}

		
	}

	void BasicTestSpaceLevel::handleTestParticleSpawn()
	{
		static std::random_device rng;
		static std::seed_seq seed{ 28 };
		static std::mt19937 rng_eng = std::mt19937(seed);
		static std::uniform_real_distribution<float> startDist(particleSpawnBounds.x, particleSpawnBounds.y + 0.01f); //[a, b)

		if(testParticleConfig)
		{
			ParticleSystem& particleSys = GameBase::get().getParticleSystem();
			for (int particleIdx = 0; particleIdx < numParticlesInBatch; ++particleIdx)
			{
				glm::vec3 spawnLocation(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));
				
				ParticleSystem::SpawnParams spawnParams;
				spawnParams.particle = testParticleConfig;
				spawnParams.xform.position = spawnLocation + particleSpawnOffset;

				particleSys.spawnParticle(spawnParams);
			}
		}
	}

	void BasicTestSpaceLevel::onCreateLocalPlanets()
{
		sp<RNG> rng = GameBase::get().getRNGSystem().getSeededRNG(13);
		
		planets.push_back(makeRandomPlanet(*rng));
		planets.push_back(makeRandomPlanet(*rng));
		planets.push_back(makeRandomPlanet(*rng));
	}

	void BasicTestSpaceLevel::onEntitySpawned_v(const sp<WorldEntity>& spawned)
	{
		if (sp<Ship> ship = std::dynamic_pointer_cast<Ship>(spawned))
		{
			spawnedShips.insert(ship);

			ship->onDestroyedEvent->addWeakObj(sp_this(), &BasicTestSpaceLevel::handleEntityDestroyed);
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

	sp<StarField> BasicTestSpaceLevel::onCreateStarField()
	{
		if (bEnableStarField)
		{
			return SpaceLevelBase::onCreateStarField();
		}
		return sp<StarField>(nullptr);
	}

	void BasicTestSpaceLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		SpaceLevelBase::render(dt_sec, view, projection);

#if SA_RENDER_DEBUG_INFO

		/////////////////////////////////////////////
		//new method of rendering debug information//
		/////////////////////////////////////////////
		DebugRenderSystem& debugRenderer = GameBase::get().getDebugRenderSystem();
		//render x,y,z at origin
		debugRenderer.renderLine(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0)); //x
		debugRenderer.renderLine(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)); //y
		debugRenderer.renderLine(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1)); //z

		/////////////////////////////////////////////
		//old method of rendering debug information//
		/////////////////////////////////////////////
		bool bShouldLoopOverShips = CollisionDebugRenderer::bRenderCollisionOBB_ui || CollisionDebugRenderer::bRenderCollisionShapes_ui;
		if (bShouldLoopOverShips)
		{
			for (const sp<Ship> ship : spawnedShips)
			{
				const CollisionData* collisionData = ship->getGameComponent<CollisionComponent>()->getCollisionData(); //we know ships have collision components should never be null.
				assert(collisionData);
				if (collisionData) //#TODO this should always be valid, but until I migrate large ships over to new system it may not be valid
				{
					glm::mat4 shipModelMat = ship->getTransform().getModelMatrix();

					collisionData->debugRender(shipModelMat, view, projection);
				}
			}
		}

#endif //SA_RENDER_DEBUG_INFO
	}

}

