#include "AIDebuggerWidget.h"
#include "GameFramework/SALevel.h"
#include "GameFramework/SALevelSystem.h"
#include "Game/SpaceArcade.h"
#include "Game/GameSystems/SAUISystem_Editor.h"
#include "Libraries/imgui.1.69.gl/imgui.h"
#include "../../../Tools/PlatformUtils.h"
#include "../../SAShip.h"
#include "../../../GameFramework/SAPlayerBase.h"
#include "../../../Rendering/Camera/SACameraBase.h"
#include "../../../GameFramework/SAPlayerSystem.h"
#include "Tools/SAUtilities.h"
#include "Game/GameSystems/SAModSystem.h"
#include "../../../GameFramework/Components/GameplayComponents.h"
#include "../../AI/SAShipAIBrain.h"
#include "../../../GameFramework/SABehaviorTree.h"
#include "SAHitboxPicker.h"
#include "../../Cheats/SpaceArcadeCheatSystem.h"
#include "../../SAPlayer.h"
#include "../../AI/SAShipBehaviorTreeNodes.h"
#include "../../AI/GlobalSpaceArcadeBehaviorTreeKeys.h"
#include "../../Components/FighterSpawnComponent.h"

namespace SA
{
	using namespace glm;


	void AIDebuggerWidget::postConstruct()
	{
		Parent::postConstruct();

		SpaceArcade& game = SpaceArcade::get();
		LevelSystem& levelSystem = game.getLevelSystem();

		game.getEditorUISystem()->onUIFrameStarted.addWeakObj(sp_this(), &AIDebuggerWidget::handleUIFrameStarted);
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &AIDebuggerWidget::handlePreLevelChange);
		if (sp<LevelBase> currentLevel = levelSystem.getCurrentLevel())
		{
			handlePreLevelChange(currentLevel, nullptr);
		}
	}


	bool AIDebuggerWidget::tick(float dt_sec)
	{
		/////////////////////////////////////////////////////////////////////////////////////
		// update camera
		/////////////////////////////////////////////////////////////////////////////////////
		if (bCameraFollowTarget && following)
		{
			static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
			const sp<PlayerBase>& player = playerSystem.getPlayer(0);
			const sp<CameraBase>& camera = player->getCamera();
			if (camera && !IsControlling(*player, *following))
			{
				vec3 followPos = following->getWorldPosition();
				vec3 oldCamPos = camera->getPosition();
				vec3 toOldCamera = oldCamPos - followPos; //used so that cross product doesn't do flips
				vec3 newCamPos = oldCamPos;
				vec3 shipUpDir_n = following->getUpDir();

				const Ship* followingTarget = getTarget(*following.fastGet());
				if (bPlanarCameraMode && followingTarget)
				{
					vec3 shipToTarget_n = normalize(followingTarget->getWorldPosition() - followPos);
					vec3 inPlaneVec_n = vec3(following->getForwardDir());
					if (glm::dot(inPlaneVec_n, shipToTarget_n) > 0.98f)
					{
						inPlaneVec_n = inPlaneVec_n * glm::angleAxis(glm::radians(1.0f), normalize(vec3(shipUpDir_n)));
					}

					//if cross is on other side from current camera, avoid flip and just use cross product in direction of current camera position
					vec3 camOffsetDir_n = normalize(glm::cross(shipToTarget_n, inPlaneVec_n));
					if (glm::dot(camOffsetDir_n, toOldCamera) < 0) 
					{
						camOffsetDir_n *= -1;
					}
					newCamPos = followPos + cameraDistance * camOffsetDir_n;
				}
				else
				{
					newCamPos = followPos + cameraDistance * -camera->getFront();
				}
				camera->setPosition(newCamPos);
				camera->lookAt_v(followPos);
			}
		}

		return true;
	}

	void AIDebuggerWidget::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		//probably not really necessary to go through all this work, as we need the level to set the hitbox picker widget
		if (currentLevel) { currentLevel->getWorldTimeManager()->removeTicker(sp_this()); }
		if (newLevel) { newLevel->getWorldTimeManager()->registerTicker(sp_this()); }
	}

	void AIDebuggerWidget::handleUIFrameStarted()
	{
		static bool bFirstCall = []()
		{
			ImGui::SetNextWindowSize(ImVec2{ 300, 330 });
			ImGui::SetNextWindowPos(ImVec2{ 1000, 20 });
			return false;
		}();
		ImGuiWindowFlags flags = 0;
		//flags |= ImGuiWindowFlags_NoMove;
		//flags |= ImGuiWindowFlags_NoResize;
		ImGui::Begin("AI debugger Tool", nullptr, flags);
		{
			ImGui::SetWindowCollapsed(true, ImGuiCond_Once);
			ImGui::TextWrapped("AI Debugger Tool");
			if (ImGui::Button("Destroy all ships"))
			{
				destroyAllShips();
			}
			if (ImGui::Button("control | release"))
			{
				togglePossessRelease(GameBase::get().getPlayerSystem().getPlayer(0), following);
			}
			if (ImGui::Button("Spawn Two Fighting Ships"))
			{
				CheatStatics::givePlayerQuaternionCamera(); //make sure we're using a smooth camera for this

				spawnTwoFightingShips([](const sp<Ship>& ship) {
					if (BrainComponent* brainComp = ship ? ship->getGameComponent<BrainComponent>() : nullptr)
					{
						brainComp->setNewBrain(new_sp<DogfightTestBrain>(ship));
					}
				});
			}
			ImGui::SameLine(); if (ImGui::Button("Swap Follow Targets"))
			{
				swapFollowTargets();
			}
			if (ImGui::Button("Spawn Fighter attacking objective"))
			{
				spawnFighterToAttackObjective();
			}
			static bool bObjectiveTweaker = false;
			ImGui::Checkbox("Objective Tweaker", &bObjectiveTweaker);
			if (bObjectiveTweaker)
			{
				ImGui::SliderFloat("attack Dist", &BehaviorTree::Task_AttackObjective::c.attackFromDistance, 0.f, 150.f);
				ImGui::SliderFloat("setup area acceptance radius", &BehaviorTree::Task_AttackObjective::c.acceptiableAttackRadius, 0.f, 150.f);
				ImGui::SliderFloat("disengage radius", &BehaviorTree::Task_AttackObjective::c.acceptiableAttackRadius, 0.f, 150.f);
			}

			static bool bPlayerDogfightTweaker = false;
			ImGui::Checkbox("Player Dogfight Tweaker", &bPlayerDogfightTweaker);
			if (bPlayerDogfightTweaker)
			{
				ImGui::SliderFloat("Viscosity Dist", &BehaviorTree::DogFightConstants::AiVsPlayer_ViscosityRange, 0.f, 150.f);
				ImGui::SliderFloat("Speed Dist", &BehaviorTree::DogFightConstants::AiVsPlayer_SpeedBoostRange, 0.f, 150.f);
				ImGui::SliderFloat("max speedup", &BehaviorTree::DogFightConstants::AiVsPlayer_MaxSpeedBoost, 1.f, 16.f);
			}
			ImGui::Checkbox("camera follow target spawned fighter", &bCameraFollowTarget);
			ImGui::Checkbox("planarCameraMode", &bPlanarCameraMode);
			if (ImGui::Checkbox("Following AI Simulate Player", &bSimulatePlayer))
			{
				refreshAISimulatingPlayer();
			}
			if (ImGui::Checkbox("bChangeBackgroundColor", &bChangeBackgroundColor))
			{
				static vec3 currentBgColor = SpaceArcade::get().getClearColor(); //first time init
				if (!bChangeBackgroundColor)
				{
					SpaceArcade::get().setClearColor(currentBgColor);
				}
				else
				{
					currentBgColor = SpaceArcade::get().getClearColor(); //cache off for untoggle
					SpaceArcade::get().setClearColor(vec3(0.5f));
				}
			}
			ImGui::SliderFloat("zoom", &cameraDistance, 1, 150);

		}
		ImGui::End();
	}

	const SA::Ship* AIDebuggerWidget::getTarget(const Ship& ship) const
	{
		const Ship* target = nullptr;
		if (const BrainComponent* brainComp = ship.getGameComponent<BrainComponent>())
		{
			if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
			{
				using namespace BehaviorTree;
				Memory& memory = behaviorTree->getMemory();
				{
					target = memory.getReadValueAs<Ship>(BT_TargetKey);

				}
			}
		}

		return target;
	}

	void AIDebuggerWidget::swapFollowTargets()
	{
		if (following)
		{
			//debug code so less sad about const_cast... ideally requestTypedReference is a const function, #TODO will attempt conversion
			if (Ship* followingTarget_raw = const_cast<Ship*>(getTarget(*following.fastGet())))
			{
				fwp<const Ship> followingTarget = followingTarget_raw->requestTypedReference_Nonsafe<const Ship>();;
				if (followingTarget)
				{
					following = followingTarget;
				}
			}
		}
	}

	void AIDebuggerWidget::refreshAISimulatingPlayer()
	{
		using namespace BehaviorTree;
		PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();

		//create a second player if necessary
		if (playerSystem.numPlayers() == 1)
		{
			sp<SAPlayer> player = playerSystem.createPlayer<SAPlayer>();
			player->setIsPlayerSimulation(true);
		}
		if (const sp<PlayerBase>& player2 = playerSystem.getPlayer(1))
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// set owning player so that BT can detect its target is owned/controlled by a player
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (bSimulatePlayer)
			{
				if (following && player2)
				{
					Ship* ship = const_cast<Ship*>(following.fastGet());//debug code

					if (OwningPlayerComponent* playerComp = createOrGetOptionalGameComponent<OwningPlayerComponent>(*ship))
					{
						playerComp->setOwningPlayer(player2);
					}
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//refresh targeting system so we don't need to poll if target is owned by a player
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (following)
			{
				Ship* followingShip = const_cast<Ship*>(following.fastGet());//this is debug code, which is why I'm allowing const cast to happen
				Ship* shipTarget = const_cast<Ship*>(getTarget(*followingShip)); //this is debug code, which is why I'm allowing const cast to happen

				const BrainComponent* brainComp = shipTarget->getGameComponent<BrainComponent>();
				if (const BehaviorTree::Tree* bt = brainComp->getTree())
				{
					Memory& memory = bt->getMemory();

					//this target is different, it is the ship we're following -- we want the enemy tageting our follow target to detech a chance and notice it is now a player
					wp<Ship> targetShip_wp = followingShip->requestTypedReference_Nonsafe<Ship>();
					sp<Ship> targetShip_sp = bSimulatePlayer ? targetShip_wp.lock() : sp<Ship>(nullptr); //null will clear out in cases where we're no longer simulating player
					memory.replaceValue(BT_TargetKey, targetShip_sp);
				}
			}
		}
	}

	void AIDebuggerWidget::togglePossessRelease(const sp<PlayerBase>& player, const fwp<const Ship>& follow) const
	{
		if (player && follow)
		{
			SA::Ship* target = const_cast<Ship*>(getTarget(*follow)); //NOTE: must get target before player posses follow ship

			if (IsControlling(*player, *follow.get()))
			{
				player->setControlTarget(nullptr);
				CheatStatics::givePlayerQuaternionCamera();
			}
			else
			{
				havePlayerControlShip(player, follow);
			}

			//make sure targeted ai gets its target refreshed so that it knows it is fighting a player
			if (target)
			{
				wp<const Ship> const_follow_wp = follow;
				sp<const Ship> const_follow_sp = const_follow_wp.lock();
				sp<Ship> follow_sp = std::const_pointer_cast<Ship>(const_follow_sp);

				if (BrainComponent* brainComp = target->getGameComponent<BrainComponent>())
				{
					if (const BehaviorTree::Tree* tree = brainComp->getTree())
					{
						using namespace BehaviorTree;
						Memory& memory = tree->getMemory();
						memory.replaceValue(BT_TargetKey, follow_sp);
					}
				}
			}
		}
	}

	void AIDebuggerWidget::havePlayerControlShip(const sp<PlayerBase>& player, const fwp<const Ship>& follow) const
	{
		wp<const Ship> const_follow_wp = follow;
		sp<const Ship> const_follow_sp = const_follow_wp.lock();
		sp<Ship> follow_sp = std::const_pointer_cast<Ship>(const_follow_sp);
		player->setControlTarget(follow_sp);
	}

	bool AIDebuggerWidget::IsControlling(PlayerBase& player, const Ship& following) const
	{
		bool bIsControlling = false;
		IControllable* controlTarget = player.getControlTarget();
		if (controlTarget)
		{
			bIsControlling = controlTarget == &following;
		}
		return bIsControlling;
	}

	void AIDebuggerWidget::destroyAllShips()
	{
		if (const sp<LevelBase>& world = SpaceArcade::get().getLevelSystem().getCurrentLevel())
		{
			const std::set<sp<WorldEntity>>& worldEntities = world->getWorldEntities();
			for (const sp<WorldEntity>& entity : worldEntities)
			{
				if(Ship* shipPtr = dynamic_cast<Ship*>(entity.get()))
				{
					shipPtr->destroy();
				}
			}

		} else {STOP_DEBUGGER_HERE(); }
	}

	void AIDebuggerWidget::spawnTwoFightingShips(std::function<void(const sp<Ship>&)> brainSpawningFunction)
	{
		const sp<LevelBase>& currentLevel = SpaceArcade::get().getLevelSystem().getCurrentLevel();
		const sp<PlayerBase>& player = SpaceArcade::get().getPlayerSystem().getPlayer(0);
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (player && currentLevel && activeMod)
		{
			if (const sp<CameraBase>& camera = player->getCamera())
			{
				const vec3 camPos = camera->getPosition();
				const vec3 camUp = camera->getUp();
				const vec3 camFront = camera->getFront();
				const float camFrontSpawnDist = 10.f;
				const float shipSeparationRadius = 5.f;

				const vec3 shipPos_A = camPos + camFront * camFrontSpawnDist + camUp * shipSeparationRadius;
				const vec3 shipPos_B = camPos + camFront * camFrontSpawnDist - camUp * shipSeparationRadius;

				const vec3 shipForward_A = glm::normalize(shipPos_B - shipPos_A);
				const vec3 shipForward_B = -shipForward_A;

				////////////////////////////////////////////////////////
				// spawn the ships
				////////////////////////////////////////////////////////
				auto spawnShip = [&](const glm::vec3& pos, const glm::vec3& forward_n, size_t teamIdx)
				{
					Ship::SpawnData spawnData;
					//find first fighter spawnable config from the team carrier config
					if (const sp<SpawnConfig>& carrierConfig = activeMod->getDeafultCarrierConfigForTeam(teamIdx))
					{
						const std::vector<std::string>& spawnableConfigsByName = carrierConfig->getSpawnableConfigsByName();
						if (spawnableConfigsByName.size() > 0)
						{
							const std::map<std::string, sp<SpawnConfig>>& nameToConfigMap = activeMod->getSpawnConfigs();
							auto findResult = nameToConfigMap.find(spawnableConfigsByName[0]);
							if (findResult != nameToConfigMap.end())
							{
								spawnData.spawnConfig = findResult->second;
							}
						}
					}
					if (!spawnData.spawnConfig) { return sp<Ship>(nullptr); } //make sure we found a spawn config

					spawnData.team = teamIdx;
					spawnData.spawnTransform.position = pos;
					spawnData.spawnTransform.rotQuat = Utils::getRotationBetween(
						normalize(spawnData.spawnConfig->getModelFacingDir_n() * spawnData.spawnConfig->getModelDefaultRotation()),	 //normalizing for safety, shouldn't be necessary analytically, may be necessarily numerically
						forward_n);//spawn ship facing space point dir

					sp<Ship> newShip = currentLevel->spawnEntity<Ship>(spawnData);
					newShip->setVelocityDir(forward_n); //this currently normalizes, so normalizing twice but leaving to avoid code fragility
		
					return newShip;
				};

				sp<Ship> shipA = spawnShip(shipPos_A, shipForward_A,/*team*/ 0);
				sp<Ship> shipB = spawnShip(shipPos_B, shipForward_B,/*team*/ 1);

				//std::vector<sp<Ship>> spawned{ shipA, shipB };
				if (shipA && shipB)
				{
					brainSpawningFunction(shipA);
					brainSpawningFunction(shipB);
				} else { STOP_DEBUGGER_HERE(); }

				auto targetOther = [](const sp<Ship>& first, const sp<Ship>& second)
				{
					if (const BrainComponent* brainComp = first->getGameComponent<BrainComponent>())
					{
						if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
						{
							using namespace BehaviorTree;
							Memory& memory = behaviorTree->getMemory();
							{
								memory.replaceValue(BT_TargetKey, second);
							}
						}
					}
				};

				targetOther(shipA, shipB);
				targetOther(shipB, shipA);

				following = shipA;
			}
		}

		refreshAISimulatingPlayer();
	}

	void AIDebuggerWidget::spawnFighterToAttackObjective()
	{
		if (bool bIsServer = true)
		{
			const sp<LevelBase>& currentLevel = SpaceArcade::get().getLevelSystem().getCurrentLevel();
			const sp<PlayerBase>& player = SpaceArcade::get().getPlayerSystem().getPlayer(0);
			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
			if (player && currentLevel && activeMod)
			{
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// find an objective
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				sp<Ship> enemyCarrier = nullptr;
				const std::set<sp<WorldEntity>>& worldEntities = currentLevel->getWorldEntities();
				for (const sp<WorldEntity>& worldEntity : worldEntities)
				{
					const FighterSpawnComponent* spawnComp = worldEntity->getGameComponent<FighterSpawnComponent>();
					if (bool bIsCarrier = (spawnComp != nullptr))
					{
						enemyCarrier = std::dynamic_pointer_cast<Ship>(worldEntity);
						if (enemyCarrier)
						{
							break;
						}
					}
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// spawn the fighter and have it target the objective
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (sp<ShipPlacementEntity> objectiveToTarget = enemyCarrier ? enemyCarrier->getRandomObjective() : nullptr)
				{
					size_t toTargetTeam = enemyCarrier->getTeam();

					//find valid team to spawn target as; would be better to use modulus but we may not have teams above so we have to go in reverse
					//avoiding that logic as this is just debug code anyways.
					size_t spawningTeam = 0;
					for (;spawningTeam < MAX_TEAM_NUM; ++spawningTeam)
					{
						if (spawningTeam != toTargetTeam){break;}
					}

					if (const sp<CameraBase>& camera = player->getCamera())
					{
						const vec3 camPos = camera->getPosition();
						const vec3 camUp = camera->getUp();
						const vec3 camFront = camera->getFront();
						const float camFrontSpawnDist = 10.f;
						const float shipSeparationRadius = 5.f;

						const vec3 shipPos_A = camPos + camFront * camFrontSpawnDist + camUp * shipSeparationRadius;

						sp<Ship> spawnedFighter = helperSpawnShip(shipPos_A, enemyCarrier->getWorldPosition() - shipPos_A,/*team*/ spawningTeam);
						if (spawnedFighter)
						{
							if (BrainComponent* brainComp = spawnedFighter ? spawnedFighter->getGameComponent<BrainComponent>() : nullptr)
							{
								brainComp->setNewBrain(new_sp<FighterBrain>(spawnedFighter));

								////////////////////////////////////////////////////////
								// target objective
								////////////////////////////////////////////////////////
								if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
								{
									using namespace BehaviorTree;
									Memory& memory = behaviorTree->getMemory();
									{
										memory.replaceValue(BT_TargetKey, objectiveToTarget);
									}
								}
							}
						}

						following = spawnedFighter;
					}
				}
			}

			//refreshAISimulatingPlayer();
		}

	}

	sp<Ship> AIDebuggerWidget::helperSpawnShip(const glm::vec3& pos, const glm::vec3& forward_n, size_t teamIdx)
	{
		const sp<LevelBase>& currentLevel = SpaceArcade::get().getLevelSystem().getCurrentLevel();
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (currentLevel && activeMod)
		{
			Ship::SpawnData spawnData;
			//find first fighter spawnable config from the team carrier config
			if (const sp<SpawnConfig>& carrierConfig = activeMod->getDeafultCarrierConfigForTeam(teamIdx))
			{
				const std::vector<std::string>& spawnableConfigsByName = carrierConfig->getSpawnableConfigsByName();
				if (spawnableConfigsByName.size() > 0)
				{
					const std::map<std::string, sp<SpawnConfig>>& nameToConfigMap = activeMod->getSpawnConfigs();
					auto findResult = nameToConfigMap.find(spawnableConfigsByName[0]);
					if (findResult != nameToConfigMap.end())
					{
						spawnData.spawnConfig = findResult->second;
					}
				}
			}
			if (!spawnData.spawnConfig) { return sp<Ship>(nullptr); } //make sure we found a spawn config

			spawnData.team = teamIdx;
			spawnData.spawnTransform.position = pos;
			spawnData.spawnTransform.rotQuat = Utils::getRotationBetween(
				normalize(spawnData.spawnConfig->getModelFacingDir_n() * spawnData.spawnConfig->getModelDefaultRotation()),	 //normalizing for safety, shouldn't be necessary analytically, may be necessarily numerically
				forward_n);//spawn ship facing space point dir

			sp<Ship> newShip = currentLevel->spawnEntity<Ship>(spawnData);
			newShip->setVelocityDir(forward_n); //this currently normalizes, so normalizing twice but leaving to avoid code fragility

			return newShip;
		}
		return nullptr;
	}

}

