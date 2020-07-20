#pragma once
#include "../Widget3D_Base.h"
#include "../../../../../Tools/DataStructures/AdvancedPtrs.h"

namespace SA
{
	class Widget3D_TextProgressBar;
	class PlayerBase;
	class IControllable;
	class WorldEntity;
	struct GameUIRenderData;

	/////////////////////////////////////////////////////////////////////////////////////
	// player status bar base -- shared functionality between player status bars
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_PlayerStatusBarBase : public Widget3D_Base
	{
		using Parent = Widget3D_Base;
	public:
		Widget3D_PlayerStatusBarBase(size_t playerIdx);
		void setBarTransform(const Transform& xform); //should be called before calling renderGameUI if there needs to be an update
		virtual void renderGameUI(GameUIRenderData& renderData) override;
		void setTextTransform(Transform xform);
	protected:
		virtual void postConstruct() override;
		const fwp<PlayerBase>& getMyPlayer() { return myPlayer; }
	private:
		void registerPlayerEvents();
		void handlePlayerControlTargetSet(IControllable* oldTarget, IControllable* newTarget);
	protected:
		virtual void onPlayerControlTargetSet(IControllable* oldTarget, IControllable* newTarget) {};
	protected:
		sp<Widget3D_TextProgressBar> textProgressBar;
		fwp<WorldEntity> myControlTarget = nullptr;
	private:
		size_t assignedPlayerIdx = 0;
		fwp<PlayerBase> myPlayer = nullptr; //warning, watch out for circular references here. wp so HUD will not create circular reference with player.
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// health bar
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_HealthBar : public Widget3D_PlayerStatusBarBase
	{
		using Parent = Widget3D_PlayerStatusBarBase;
	public:
		Widget3D_HealthBar(size_t playerIdx);
		virtual void renderGameUI(GameUIRenderData& renderData);
	protected:
		virtual void postConstruct() override;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// energy bar
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_EnergyBar : public Widget3D_PlayerStatusBarBase
	{
		using Parent = Widget3D_PlayerStatusBarBase;
	public:
		Widget3D_EnergyBar(size_t playerIdx);
		virtual void renderGameUI(GameUIRenderData& renderData);
	protected:
		virtual void postConstruct() override;
	};
}

