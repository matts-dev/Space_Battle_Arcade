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
		}
		else
		{
			STOP_DEBUGGER_HERE(); //currently expecting all control targets to be world entities, did this change? feel free to remove this.
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
		textProgressBar->myProgressBar->setSlantRotation_rad(glm::radians<float>(HEALTH_BAR_SLANT_RAD));
		textProgressBar->myProgressBar->setLeftToRight(true);
	}

	void Widget3D_HealthBar::renderGameUI(GameUIRenderData& rd)
	{

		if (const HitPointComponent* hpComp = myControlTarget ? myControlTarget->getGameComponent<HitPointComponent>() : nullptr)
		{
			const HitPoints& hp = hpComp->getHP();
			textProgressBar->myProgressBar->setProgressOnRange(hp.current, 0, hp.max);
		}
		else
		{
			textProgressBar->myProgressBar->setProgressNormalized(0);//can't get health comp, show 0
		}

		Parent::renderGameUI(rd);
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
		textProgressBar->myProgressBar->setSlantRotation_rad(glm::radians<float>(-HEALTH_BAR_SLANT_RAD));
		textProgressBar->myProgressBar->setLeftToRight(false);
	}

	void Widget3D_EnergyBar::renderGameUI(GameUIRenderData& rd)
	{
		if (const ShipEnergyComponent* energyComp = myControlTarget ? myControlTarget->getGameComponent<ShipEnergyComponent>() : nullptr)
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
