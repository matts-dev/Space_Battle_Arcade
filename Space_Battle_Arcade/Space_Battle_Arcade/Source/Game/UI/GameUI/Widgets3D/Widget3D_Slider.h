#pragma once

#include <array>

#include "Game/UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ActivatableBase.h"
#include "Game/GameSystems/SAUISystem_Game.h"
#include "Tools/color_utils.h"
#include "Game/UI/GameUI/text/DigitalClockFont.h" //for enum class EVerticalPivot  because it is not possible to forward declare it since it is class internal member.

namespace SA
{
	class GlitchTextFont;
	class LaserUIObject;
	class LevelBase;

	namespace Widget3D_Slider_Utils
	{
		std::string defaultSliderToString(float value);
	}

	class Widget3D_Slider : public Widget3D_ActivatableBase, public IMouseInteractable
	{
		using Parent = Widget3D_ActivatableBase;
	public:
		virtual ~Widget3D_Slider();
	public:
		virtual void onActivationChanged(bool bActive) override;
		float getValue() const { return currentValueAlpha * valueScalar; }
		void setValue(float newValue); //[0,1] accepted range
		void setStart(const glm::vec3& start);
		void setEnd(const glm::vec3& end);
		std::array<glm::vec4, 8> getOBB();
		void setTitleText(const std::string titleStr);
		void setTitleTextScale(float newScale);
		//void setHorizontalPivot(DigitalClockFont::EHorizontalPivot& pivot);
		//void setVerticalPivot(DigitalClockFont::EVerticalPivot& pivot);
	protected:
		virtual void postConstruct() override;
		virtual void renderGameUI(GameUIRenderData& renderData);
		void handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
		virtual void tick_postCamera(float dt_sec);
	private:
		virtual void onDeactivated();
		virtual void onActivated();
		void refresh();
		void updateLaserPositions(bool bResetAnimations = true);
		void updateValueTextStr();
		void updateTextPosition(const glm::vec3& sliderUp_n, const glm::quat& camRot);
		//IMouseInteractable interface
		virtual glm::mat4 getModelMatrix() const;
		virtual const std::array<glm::vec4, 8>& getLocalAABB() const;
		virtual glm::vec3 getWorldLocation() const;;
		virtual bool isHitTestable() const { return isActive(); }
		virtual void onHoveredhisTick() override;
		virtual void onClickedThisTick() override;
		virtual bool supportsMouseDrag() const override{ return true; } 
		virtual void onMouseDraggedThisTick(const glm::vec3& worldMousePoint) override;
		virtual void onMouseDraggedReleased() override;
		//end IMouseInteractableInterface
		void updateSliderPosition();
		void updateCollisionData();
		void setSliderColor(glm::vec3 color);
	private: //configuration
		float baseLaserAnimSpeedSec = 1.f;
		float edgeSizeFactor = 0.25f; //like the edge height, but since sliders may be vertical avoiding calling it a height
		float valueScalar = 1.f; //setting this value to 5 will make slider on range [0,5] instead of [0,1]
		float sliderBoxWidth = 0.25f;
		float valueTextScale = 0.2f;
		float titleTextScale = 0.2f;
		glm::vec2 textPadding{ 0.25f, 0.25f };
		glm::vec3 hoverColor = color::metalicgold();
		glm::vec3 dragColor = color::lightYellow();
		glm::vec3 defaultColor = color::red();
		std::function<std::string(float)> toStringFunc = &Widget3D_Slider_Utils::defaultSliderToString;
	private: //IMouseInteractableState
		bool bHoveredThisTick = false;
		bool bDragging = false;
	private:
		float currentValueAlpha = 0.5f;	//[0,1] represents how far this slider is. This must be [0,1] for internal reasons like positioning.
	private:
		glm::vec3 sliderStartPoint;
		glm::vec3 sliderEndPoint;
		sp<GlitchTextFont> valueText;
		sp<GlitchTextFont> titleText;

		sp<LaserUIObject> laser_rangeBar;
		sp<LaserUIObject> laser_startEdge;
		sp<LaserUIObject> laser_endEdge;
		sp<LaserUIObject> laser_sliderBoxTop;
		sp<LaserUIObject> laser_sliderBoxLeft;
		sp<LaserUIObject> laser_sliderBoxBottom;
		sp<LaserUIObject> laser_sliderBoxRight;

		//fields related to being hittestable in UI system
		up<SH::HashEntry<IMouseInteractable>> hashEntry = nullptr;
		std::array<glm::vec4, 8> OBB; //oriented bounding box
		std::array<glm::vec4, 8> localAABB; //oriented bounding box
		glm::mat4 sliderBoxModelMat;
		glm::vec3 worldSliderBoxPos;
	};


}
