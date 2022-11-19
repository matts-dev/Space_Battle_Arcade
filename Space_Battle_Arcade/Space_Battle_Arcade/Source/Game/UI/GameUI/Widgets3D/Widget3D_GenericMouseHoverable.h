#pragma once

#include <array>
#include <string>

#include "Game/UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ActivatableBase.h"
#include "Game/GameSystems/SAUISystem_Game.h"
#include "Tools/color_utils.h"
#include "Tools/DataStructures/SATransform.h"

namespace SA
{
	class GlitchTextFont;
	class LaserUIObject;
	class LevelBase;

	namespace Widget3D_Slider_Utils
	{
		std::string defaultSliderToString(float value);
	}

	class Widget3D_GenericMouseHoverable : public Widget3D_ActivatableBase, public IMouseInteractable
	{
		using Parent = Widget3D_ActivatableBase;
	public:
		Widget3D_GenericMouseHoverable();
		virtual ~Widget3D_GenericMouseHoverable();
	public:
		const Transform& getXform() { return xform; }
		void setXform(const Transform& inXform);
		std::array<glm::vec4, 8> getOBB() const; //#todo rework this to be an out variable
		void setToggled(bool bInValue) { bToggled = bInValue; }
		bool isToggled() { return bToggled; }
	protected:
		virtual void postConstruct() override;
		//virtual void renderGameUI(GameUIRenderData& renderData);
		//virtual void tick_postCamera(float dt_sec);
		//virtual void onActivationChanged(bool bActive) { if (bActive) { onActivated() } else { onDeactivated(); } }
	private:
		//IMouseInteractable interface
		virtual glm::mat4 getModelMatrix() const;
		virtual const std::array<glm::vec4, 8>& getLocalAABB() const;
		virtual glm::vec3 getWorldLocation() const;;
		virtual bool isHitTestable() const { return isActive(); }
		virtual void onHoveredhisTick() override;
		virtual void onClickedThisTick() override;
		//virtual bool supportsMouseDrag() const override { return true; }
		//virtual void onMouseDraggedThisTick(const glm::vec3& worldMousePoint) override;
		//virtual void onMouseDraggedReleased() override;
		//end IMouseInteractableInterface
	//private:
	//	void onActivated();
	//	void onDeactivated();
	private:
		virtual void renderGameUI(GameUIRenderData& renderData);
		void updateCollisionData();
		//void handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
		void releaseLasers();
		void claimLasers();
	public:
		MultiDelegate<> onClicked;
	public: //configuration
		float radius = 0.5f; //mouse interaction radius
		bool bClickToggles = false;
	private: //configuration
		float rotationSpeedSec_rad = glm::radians<float>(90.f);
		size_t numLasers = 8;
		float laserSpeedSecs = 1.f;
	private: //IMouseInteractableState
		bool bHoveredThisTick = false;
		//bool bDragging = false;
		float rotation_rad = 0.f;
		bool bToggled = false;
	private:
		std::vector<sp<LaserUIObject>> lasers;
		Transform xform;

		//fields related to being hit testable in UI system
		up<SH::HashEntry<IMouseInteractable>> hashEntry = nullptr;
		std::array<glm::vec4, 8> OBB; //oriented bounding box
		std::array<glm::vec4, 8> localAABB; //oriented bounding box
		//glm::mat4 modelMat;
		//glm::vec3 worldPos;
	};
}
