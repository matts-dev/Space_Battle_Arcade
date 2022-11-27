#pragma once
#include <glm/detail/setup.hpp>
#include "Game/UI/GameUI/Widgets3D/Widget3D_Base.h"
#include "Game/UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ActivatableBase.h"
#include "Tools/DataStructures/AdvancedPtrs.h"

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
	class Widget3D_PlayerStatusBarBase : public Widget3D_ActivatableBase
	{
		using Parent = Widget3D_ActivatableBase;
	public:
		Widget3D_PlayerStatusBarBase(size_t playerIdx);
		void setBarTransform(const Transform& xform); //should be called before calling renderGameUI if there needs to be an update
		void setTextTransform(Transform xform);
		virtual void renderGameUI(GameUIRenderData& renderData) override;
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive);
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

	/////////////////////////////////////////////////////////////////////////////////////
	// team health bar
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_TeamProgressBar : public Widget3D_PlayerStatusBarBase
	{
		using Parent = Widget3D_PlayerStatusBarBase;
	public:
		Widget3D_TeamProgressBar(size_t playerIdx, size_t teamIdx);
	protected:
		virtual void postConstruct() override;
	public:
		virtual void renderGameUI(GameUIRenderData& renderData);
	private:
		void cacheGameMode();
		void handlePostLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& /*newCurrentLevel*/);
	private:
		size_t teamIdx;
		fwp<class ServerGameMode_SpaceBase> cacheGM = nullptr;
	};
}

