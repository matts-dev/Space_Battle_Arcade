#include "../../../../GameSystems/SAUISystem_Game.h"
#include "Widget3D_PlayerStatusBarBase.h"

#include "../Widget3D_ProgressBar.h"
#include "../../../../../GameFramework/SAPlayerBase.h"
#include "../../../../../GameFramework/SAGameBase.h"
#include "../../../../../GameFramework/SAPlayerSystem.h"

#include "../../text/GlitchText.h"
#include "../../../../../Tools/PlatformUtils.h"
#include "../../../../../GameFramework/Interfaces/SAIControllable.h"
#include "../../../../../GameFramework/SAWorldEntity.h"
#include "../../../../../GameFramework/Components/GameplayComponents.h"
#include "../../../../Components/ShipEnergyComponent.h"
#include "../../../../../GameFramework/SALevel.h"
#include "../../../../Levels/SASpaceLevelBase.h"
#include "../../../../../GameFramework/SALevelSystem.h"
#include "../../../../GameModes/ServerGameMode_SpaceBase.h"
#include "../../../../../Tools/SAUtilities.h"
#include "../../../../GameSystems/SAModSystem.h"
#include "../../../../SpaceArcade.h"
namespace SA
{
	Widget3D_PlayerStatusBarBase::Widget3D_PlayerStatusBarBase(size_t playerIdx)
		: assignedPlayerIdx(playerIdx)
	{

	}

	void Widget3D_PlayerStatusBarBase::renderGameUI(GameUIRenderData& rd_ui)
	{
		textProgressBar->renderGameUI(rd_ui);
	}

	void Widget3D_PlayerStatusBarBase::setTextTransform(Transform xform)
	{
		textProgressBar->myText->setXform(xform);
	}

	void Widget3D_PlayerStatusBarBase::postConstruct()
	{
		Parent::postConstruct();

		textProgressBar = new_sp<Widget3D_TextProgressBar>();

		registerPlayerEvents();
	}

	void Widget3D_PlayerStatusBarBase::onActivationChanged(bool bActive)
	{
		textProgressBar->myProgressBar->activate(bActive);
	}

	void Widget3D_PlayerStatusBarBase::registerPlayerEvents()
	{
		if (myPlayer)
		{
			myPlayer->onControlTargetSet.removeWeak(sp_this(), &Widget3D_PlayerStatusBarBase::handlePlayerControlTargetSet);
		}

		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(assignedPlayerIdx))
		{
			myPlayer = player;
			player->onControlTargetSet.addWeakObj(sp_this(), &Widget3D_PlayerStatusBarBase::handlePlayerControlTargetSet);
		}
	}

	void Widget3D_PlayerStatusBarBase::handlePlayerControlTargetSet(IControllable* oldTarget, IControllable* newTarget)
	{
		if (WorldEntity* controlTarget_we = newTarget ? newTarget->asWorldEntity() : nullptr)
		{
			myControlTarget = controlTarget_we->requestTypedReference_Safe<WorldEntity>();
			activate(true);
		}
		else
		{
			activate(false);
			if (newTarget)
			{
				STOP_DEBUGGER_HERE(); //currently expecting all control targets to be world entities, did this change? feel free to remove this.
			}
		}

	}

	void Widget3D_PlayerStatusBarBase::setBarTransform(const Transform& xform)
	{
		textProgressBar->myProgressBar->setTransform(xform);
	}

	const float HEALTH_BAR_SLANT_RAD = glm::radians<float>(33.f);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Health Bar
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Widget3D_HealthBar::Widget3D_HealthBar(size_t playerIdx)
		: Parent(playerIdx)
	{
		
	}

	void Widget3D_HealthBar::postConstruct()
	{
		Parent::postConstruct();
		textProgressBar->myText->setText("Health");
		textProgressBar->myProgressBar->setSlantRotation_rad(HEALTH_BAR_SLANT_RAD);
		textProgressBar->myProgressBar->setLeftToRight(true);
	}

	void Widget3D_HealthBar::renderGameUI(GameUIRenderData& rd)
	{
		if (myControlTarget)
		{
			if (const HitPointComponent* hpComp = myControlTarget->getGameComponent<HitPointComponent>() )
			{
				const HitPoints& hp = hpComp->getHP();
				textProgressBar->myProgressBar->setProgressOnRange(hp.current, 0, hp.max);
			}
			else
			{
				textProgressBar->myProgressBar->setProgressNormalized(0);//can't get health comp, show 0
			}
			Parent::renderGameUI(rd); //only render if we have a control target.
		}
	}


	////////////////////////////////////////////////////////
	// energy bar
	////////////////////////////////////////////////////////
	Widget3D_EnergyBar::Widget3D_EnergyBar(size_t playerIdx)
		: Parent(playerIdx)
	{

	}

	void Widget3D_EnergyBar::postConstruct()
	{
		Parent::postConstruct();
		textProgressBar->myText->setText("Energy");
		textProgressBar->myProgressBar->setSlantRotation_rad(-HEALTH_BAR_SLANT_RAD);
		textProgressBar->myProgressBar->setLeftToRight(false);
	}

	void Widget3D_EnergyBar::renderGameUI(GameUIRenderData& rd)
	{
		if (myControlTarget)
		{
			if (const ShipEnergyComponent* energyComp = myControlTarget->getGameComponent<ShipEnergyComponent>() )
			{
				float energy = energyComp->getEnergy();
				float maxEnergy = energyComp->getMaxEnergy();
				textProgressBar->myProgressBar->setProgressOnRange(energy, 0, maxEnergy);
			}
			else
			{
				textProgressBar->myProgressBar->setProgressNormalized(0);//can't get enegy component, show 0
			}
			Parent::renderGameUI(rd);
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// team health/progress bar
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Widget3D_TeamProgressBar::Widget3D_TeamProgressBar(size_t playerIdx, size_t inTeamIdx) : Parent(playerIdx), teamIdx(inTeamIdx)
	{

	}

	void Widget3D_TeamProgressBar::postConstruct()
	{
		Parent::postConstruct();

		//TODO_lookup_fighter_for_team_and_cache_color_for_lasers;
		if (!cacheGM) //#multiplayer clients are currently reading from gamemode, need to refactor this to read from gamestate
		{
			if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
			{
				//dynamic cast will only happen until we get a gamemode, which should be first frame
				if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
				{
					if (ServerGameMode_SpaceBase* serverGameMode = spaceLevel->getServerGameMode_SpaceBase())
					{
						cacheGM = serverGameMode->requestTypedReference_Nonsafe<ServerGameMode_SpaceBase>();
					}
				}
			}
		}

		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			if (activeMod->teamHasName(teamIdx))
			{
				textProgressBar->myText->setText(activeMod->getTeamName(teamIdx));
			}
			else
			{
				char teamString[2048];
				snprintf(teamString, sizeof(teamString), "Team %d", teamIdx + 1);
				textProgressBar->myText->setText(teamString);
			}
		}
		textProgressBar->myProgressBar->setSlantRotation_rad(0.f);
	}

	void Widget3D_TeamProgressBar::renderGameUI(GameUIRenderData& renderData)
	{
		if (cacheGM)
		{
			const std::vector<GameModeTeamData> gmTeamData = cacheGM->getTeamData();
			if (Utils::isValidIndex(gmTeamData, teamIdx))
			{
				const GameModeTeamData& td = gmTeamData[teamIdx];
				textProgressBar->myProgressBar->setProgressNormalized(td.percentAlive_Objectives);
			}
			else
			{
				textProgressBar->myText->setText("Invalid team idx");
				textProgressBar->myProgressBar->setProgressNormalized(0.f);
			}
		}
		else
		{
			textProgressBar->myText->setText("no gamestate data");
		}

		Parent::renderGameUI(renderData);
	}

}
