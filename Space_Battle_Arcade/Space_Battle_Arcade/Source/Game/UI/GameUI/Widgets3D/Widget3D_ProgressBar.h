#pragma once
#include "Game/UI/GameUI/Widgets3D/Widget3D_Base.h"
#include "Tools/DataStructures/SATransform.h"
#include "Game/UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ActivatableBase.h"


namespace SA
{
	class LaserUIObject;
	class GlitchTextFont;

	/////////////////////////////////////////////////////////////////////////////////////
	// A widget that represents a progress bar. 
	//
	// API:
	//		update the progress values if they change
	//		update the xform with the new location before calling render
	//		User should manually call renderGameUI function after updating xform
	//			this non-automatic behavior lets progress bars be configured to stay
	//			relative to camera at custom configurations.
	//		update slant rotation if you prefer a parallelogram to a box
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_ProgressBar : public Widget3D_ActivatableBase
	{
	public:
		Widget3D_ProgressBar(bool bInCameraRelative = true) : bCameraRelative(bInCameraRelative)
		{}
	public:
		virtual void renderGameUI(GameUIRenderData& renderData) override;
		virtual void postConstruct() override;
		void setProgressNormalized(float inValue);
		void setProgressOnRange(float value, float min, float max);
		void setSlantRotation_rad(float slantRot_rad) { barSlantRotation_rad = slantRot_rad; }
		void setLeftToRight(bool bProgressFromLeft) { bLeftToRight = bProgressFromLeft; }
		void setTransform(const Transform& inXform) { xform = inXform; }
	protected:
		virtual void onActivationChanged(bool bActive) override;
	private:
		void updateLasers(bool bResetAnimation);
		bool hasLasers();
	public:
		float valueScale_visualAdjustment = 0.99f; //below zero values will cause maximum capacity to be slightly lower than 100% to communicate which direction is direction of progress
	private:
		float normalizedValue = 0.f; 
		float barSlantRotation_rad = glm::radians<float>(33.f);
		Transform xform;
		bool bLeftToRight = true;
		bool bCameraRelative; //set by ctor so no default value
	private:
		sp<LaserUIObject> laser_rangeBar;
		sp<LaserUIObject> laser_progressVerticalBar;
		sp<LaserUIObject> laser_boxTop;
		sp<LaserUIObject> laser_boxLeft;
		sp<LaserUIObject> laser_boxBottom;
		sp<LaserUIObject> laser_boxRight;
		std::vector<sp<LaserUIObject>> laserCollection;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// A progress bar with text above it, see notes about progress bar
	// This is a very thin composition class. most of its members are exposed but gives 
	// you some behavior for free.
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_TextProgressBar : public Widget3D_Base
	{
	public:
		enum class AutomaticTextPositionMode{ TOP, BOTTOM };

		virtual void renderGameUI(GameUIRenderData& renderData) override;
		virtual void postConstruct() override;
		//void offsetTextPosition();
	public:
		//struct AutomaticTextPositioningData
		//{
		//	AutomaticTextPositionMode textMode = AutomaticTextPositionMode::TOP;
		//	float offsetDistance = 1.f;
		//} textData;
	public:
		sp<Widget3D_ProgressBar> myProgressBar;
		sp<GlitchTextFont> myText;
	};

}