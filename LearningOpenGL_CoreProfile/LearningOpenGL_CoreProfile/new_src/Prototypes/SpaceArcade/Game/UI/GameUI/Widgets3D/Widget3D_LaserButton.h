#include "MainMenuScreens/Widget3D_ActivatableBase.h"

#include "../../../../GameFramework/Interfaces/SATickable.h"
#include <string>

namespace SA
{
	class GlitchTextFont;
	class LaserUIObject;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Stylized button that renders using lasers
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_LaserButton : public Widget3D_ActivatableBase, public ITickable
	{
	public:
		Widget3D_LaserButton(const std::string& text = "Laser Button");
		void setText(const std::string& text);
		void setXform(const Transform& xform);
	public:
		MultiDelegate<> OnClickedDelegate;
	protected:
		virtual void postConstruct() override;
	private:
		virtual void renderGameUI(GameUIRenderData& renderData) override;
		virtual bool tick(float dt_sec) override;
	private:
		void changeLaserDelegateSubscription(LaserUIObject& laser, bool bSubscribe);
		virtual void onActivationChanged(bool bActive) override;
		void onActivated();
		void onDeactivated();
		void updateLaserPositions();
	private:
		float widthPaddingFactor = 1.25f;
		float heightPaddingFactor = 1.25f;
	private:
		std::string textCache;
		sp<GlitchTextFont> myGlitchText;
		Transform myXform;

		sp<LaserUIObject> topLaser = nullptr;
		sp<LaserUIObject> bottomLaser = nullptr;
		sp<LaserUIObject> leftLaser = nullptr;
		sp<LaserUIObject> rightLaser = nullptr;
	};
}