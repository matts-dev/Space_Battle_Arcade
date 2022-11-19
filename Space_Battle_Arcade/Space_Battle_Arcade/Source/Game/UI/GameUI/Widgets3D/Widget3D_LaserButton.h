#pragma once

#include "MainMenuScreens/Widget3D_ActivatableBase.h"

#include <string>
#include <limits>

#include "../../../GameSystems/SAUISystem_Game.h"
#include "../../../../GameFramework/Interfaces/SATickable.h"
#include "Tools/color_utils.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"

namespace SA
{
	class GlitchTextFont;
	class LaserUIObject;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Stylized button that renders using lasers
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_LaserButton : public Widget3D_ActivatableBase, public ITickable, public IMouseInteractable
	{
		using Parent = Widget3D_ActivatableBase;
	public:
		Widget3D_LaserButton(const std::string& text = "Laser Button");
		virtual ~Widget3D_LaserButton();
		void setText(const std::string& text);
		void setXform(const Transform& xform);
		const Transform& getXform();
		glm::vec2 getSize() const;
		glm::vec2 getPadding() const;
		glm::vec2 getPaddedSize() const;
		virtual bool tick(float dt_sec) override;
	public: //IMouseInteractable
		virtual glm::mat4 getModelMatrix() const override;
		virtual const std::array<glm::vec4, 8>& getLocalAABB() const override;
		virtual glm::vec3 getWorldLocation() const override;
		std::array<glm::vec4, 8> getOBB();
		virtual bool isHitTestable() const override;
	public:
		MultiDelegate<> onClickedDelegate;
	protected:
		virtual void postConstruct() override;
	protected://IMouseInteractable
		virtual void onHoveredhisTick() override;
		virtual void onClickedThisTick() override;
	private:
		virtual void renderGameUI(GameUIRenderData& renderData) override;
		//void changeLaserDelegateSubscription(LaserUIObject& laser, bool bSubscribe);
		virtual void onActivationChanged(bool bActive) override;
		void onActivated();
		void onDeactivated();
		void updateLaserPositions();
		void handleNewTextDataSet();
		void updateLocalAABB();
		void updateCollisionData();
		void setFontColor(const glm::vec3& color);
	private: //colors
		glm::vec3 activateColor = SA::color::red();
		glm::vec3 hoverColor = SA::color::lightYellow();
		glm::vec3 clickedColor = glm::vec3(1.f);
	private:
		const float widthPaddingFactor = 1.25f; //if setters are added for padding fields, the aabb will need to be regenerated.
		const float heightPaddingFactor = 1.25f;
		float baseLaserAnimSpeedSec = 1.0f;
		float clickResetDelaySec = 0.05f;
	private:
		bool bHoveredThisTick = false;
		bool bClickedThisTick = false;
		float accumulatedTickSec = 0.f;
		float lastButtonClickSec = -std::numeric_limits<float>::infinity();
	private:
		std::array<glm::vec4, 8> localAABB;
		std::string textCache;
		sp<GlitchTextFont> myGlitchText;

		sp<class AudioEmitter> hoverSound = nullptr;

		up<SH::HashEntry<IMouseInteractable>> hashEntry = nullptr;
		sp<LaserUIObject> topLaser = nullptr;
		sp<LaserUIObject> bottomLaser = nullptr;
		sp<LaserUIObject> leftLaser = nullptr;
		sp<LaserUIObject> rightLaser = nullptr;
	};
}