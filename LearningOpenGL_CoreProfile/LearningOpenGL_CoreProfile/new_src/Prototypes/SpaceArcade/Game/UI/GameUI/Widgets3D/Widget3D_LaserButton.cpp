#include "Widget3D_LaserButton.h"
#include "../LaserUIPool.h"
#include "../text/DigitalClockFont.h"
#include "../text/GlitchText.h"
#include "../../../GameSystems/SAUISystem_Game.h"
#include "../../../../Rendering/RenderData.h"
#include "../../../../GameFramework/SADebugRenderSystem.h"
#include "../../../../GameFramework/SAGameBase.h"

namespace SA
{
	Widget3D_LaserButton::Widget3D_LaserButton(const std::string& text /*= "Laser Button"*/)
	{
		textCache = text;
	}

	void Widget3D_LaserButton::setText(const std::string& text)
	{
		myGlitchText->setText(text);
		textCache = text;
	}

	void Widget3D_LaserButton::setXform(const Transform& xform)
	{
		myXform = xform;
		myGlitchText->setXform(myXform);
		updateLaserPositions();
	}

	glm::vec2 Widget3D_LaserButton::getSize() const
	{
		if (myGlitchText)
		{
			return glm::vec2(myGlitchText->getWidth(), myGlitchText->getHeight());
		}
		return glm::vec2(0.f);
	}

	void Widget3D_LaserButton::postConstruct()
	{
		setRenderWithGameUIDispatch(true);


		DigitalClockFont::Data textInit;
		textInit.text = textCache;
		myGlitchText = new_sp<GlitchTextFont>(textInit);

		myGlitchText->setAnimPlayForward(true); //this will make glyphs invisible until this button is activated.
		myGlitchText->play(false);
	}

	void Widget3D_LaserButton::renderGameUI(GameUIRenderData& ui_rd)
	{
		if (const RenderData* game_rd = ui_rd.renderData())
		{
			myGlitchText->render(*game_rd);
		}
	}

	bool Widget3D_LaserButton::tick(float dt_sec)
	{
		myGlitchText->tick(dt_sec);

		return true;
	}

	void Widget3D_LaserButton::changeLaserDelegateSubscription(LaserUIObject& laser, bool bSubscribe)
	{
		//#TODO this may not be needed and can be removed
		if (bSubscribe)
		{
			//add subscription
		}
		else
		{
			//remove subscription
		}
	}

	void Widget3D_LaserButton::onActivationChanged(bool bActive)
	{
		if (bActive) 
		{ 
			onActivated();
		}
		else 
		{ 
			onDeactivated();
		}
	}

	void Widget3D_LaserButton::onActivated()
	{
		LaserUIPool& pool = LaserUIPool::get();

		assert(!topLaser && !bottomLaser && !leftLaser && !rightLaser); //at this point, we should not have any lasers active

		topLaser = pool.requestLaserObject();
		bottomLaser = pool.requestLaserObject();
		leftLaser = pool.requestLaserObject();
		rightLaser = pool.requestLaserObject();

		changeLaserDelegateSubscription(*topLaser, true);
		changeLaserDelegateSubscription(*bottomLaser, true);
		changeLaserDelegateSubscription(*leftLaser, true);
		changeLaserDelegateSubscription(*rightLaser, true);

		updateLaserPositions();

		myGlitchText->setAnimPlayForward(true);
		myGlitchText->play(true);
	}

	void Widget3D_LaserButton::onDeactivated()
	{
		LaserUIPool& pool = LaserUIPool::get();

		assert(topLaser && bottomLaser && leftLaser && rightLaser);

		changeLaserDelegateSubscription(*topLaser, false);
		changeLaserDelegateSubscription(*bottomLaser, false);
		changeLaserDelegateSubscription(*leftLaser, false);
		changeLaserDelegateSubscription(*rightLaser, false);

		pool.releaseLaser(topLaser);
		pool.releaseLaser(bottomLaser);
		pool.releaseLaser(leftLaser);
		pool.releaseLaser(rightLaser);

		myGlitchText->setAnimPlayForward(false);
		myGlitchText->resetAnim();
	}

	void Widget3D_LaserButton::updateLaserPositions()
	{
		using namespace glm;

		if (topLaser && bottomLaser && leftLaser && rightLaser && myGlitchText)
		{
			float halfWidth = myGlitchText->getWidth() / 2.0f; 
			float halfHeight = myGlitchText->getHeight() / 2.0f;

			halfWidth += widthPaddingFactor * myXform.scale.x;
			halfHeight += heightPaddingFactor * myXform.scale.y;

			vec3 myPos = myXform.getModelMatrix() *  vec4(0, 0, 0, 1);

			topLaser->randomizeAnimSpeed(baseLaserAnimSpeed); //prevent interpolation from looking rigid (without this, a line moving could potentially maintain parallel to screen box)
			topLaser->animateStartTo(myPos + vec3(halfWidth, halfHeight, 0));
			topLaser->animateEndTo(myPos + vec3(-halfWidth, halfHeight, 0));

			bottomLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			bottomLaser->animateStartTo(myPos + vec3(halfWidth, -halfHeight, 0));
			bottomLaser->animateEndTo(myPos + vec3(-halfWidth, -halfHeight, 0));

			rightLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			rightLaser->animateStartTo(myPos + vec3(halfWidth, halfHeight, 0));
			rightLaser->animateEndTo(myPos + vec3(halfWidth, -halfHeight, 0));

			leftLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			leftLaser->animateStartTo(myPos + vec3(-halfWidth, halfHeight, 0));
			leftLaser->animateEndTo(myPos + vec3(-halfWidth, -halfHeight, 0));
		}
	}

}

