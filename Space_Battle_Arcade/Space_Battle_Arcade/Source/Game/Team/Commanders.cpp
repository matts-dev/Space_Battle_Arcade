#include "Commanders.h"
#include "Game/SpaceArcade.h"
#include "Game/Levels/SASpaceLevelBase.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAWorldEntity.h"
#include "Game/Components/FighterSpawnComponent.h"

namespace SA
{
	TeamCommander::TeamCommander(size_t team)
	{
		createGameComponent<TeamComponent>();
		cachedTeamId = team;

		TeamComponent* teamCom = getGameComponent<TeamComponent>();
		assert(teamCom);
		if (teamCom)
		{
			teamCom->setTeam(team);
		}
	}

	sp<WorldEntity> TeamCommander::getTargetOnTeam(size_t team)
	{
		if (pendingTargetsByTeam.size() >= team + 1)
		{
			std::stack<lp<WorldEntity>>& targetStack = pendingTargetsByTeam[team];
			
			lp<WorldEntity> target;
			while (!target && targetStack.size() > 0)
			{
				target = targetStack.top();
				targetStack.pop();
			}

			if (target)
			{
				return target.toSP();
			}
		}

		return sp<WorldEntity>(nullptr);
	}

	bool TeamCommander::queueTarget(const sp<WorldEntity>& target)
	{
		TeamComponent* myTeamComp = getGameComponent<TeamComponent>();
		if (TeamComponent* spawnTeamCom = target->getGameComponent<TeamComponent>())
		{
			size_t spawnedTeam = spawnTeamCom->getTeam();
			if (spawnedTeam != myTeamComp->getTeam())
			{
				if (pendingTargetsByTeam.size() < spawnedTeam + 1)
				{
					pendingTargetsByTeam.resize(spawnedTeam + 1);
				}

				std::stack<lp<WorldEntity>>& targets = pendingTargetsByTeam[spawnedTeam];
				targets.push(target);

				return true;
			}
		}

		return false;
	}

	void TeamCommander::handleCarrierSpawned(const sp<WorldEntity>& carrierEntity)
	{
		TeamComponent* myTeamComp = getGameComponent<TeamComponent>();
		if (TeamComponent* spawnTeamCom = carrierEntity->getGameComponent<TeamComponent>())
		{
			size_t spawnedTeam = spawnTeamCom->getTeam();
			if (spawnedTeam == myTeamComp->getTeam())
			{
				fwp<WorldEntity> weakCarrier = carrierEntity;
				carriers.push_back(weakCarrier);
			}
		}
	}

	sp<SA::WorldEntity> TeamCommander::getTarget()
	{
		for (size_t teamId = 0; teamId < pendingTargetsByTeam.size(); ++teamId)
		{
			if (teamId == cachedTeamId) continue;

			if (sp<WorldEntity> target = getTargetOnTeam(teamId))
			{
				return target;
			}
		}

		return nullptr;
	}

	void TeamCommander::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();

		LevelSystem& levelSystem = game.getLevelSystem();
		const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();

		if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
		{
			spaceLevel->onSpawnedEntity.addWeakObj(sp_this(), &TeamCommander::handleEntitySpawned);

			for (const sp<WorldEntity>& entity : spaceLevel->getWorldEntities())
			{
				//stack up the target data
				handleEntitySpawned(entity);
			}
		}
	}

	void TeamCommander::handleEntitySpawned(const sp<WorldEntity>& spawned)
	{
		if (bool bIsCarrier = spawned && spawned->hasGameComponent<FighterSpawnComponent>())
		{
			handleCarrierSpawned(spawned);
		}
		else
		{
			queueTarget(spawned);
		}
	}

}

