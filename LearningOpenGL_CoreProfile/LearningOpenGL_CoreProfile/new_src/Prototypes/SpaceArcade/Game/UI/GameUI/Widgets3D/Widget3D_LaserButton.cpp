#include "Widget3D_LaserButton.h"
#include "../LaserUIPool.h"
#include "../text/DigitalClockFont.h"
#include "../text/GlitchText.h"
#include "../../../GameSystems/SAUISystem_Game.h"
#include "../../../../Rendering/RenderData.h"
#include "../../../../GameFramework/SADebugRenderSystem.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../SpaceArcade.h"
#include "../../../../Tools/PlatformUtils.h"

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
		myGlitchText->setXform(xform);
		myGlitchText->onNewTextDataBuilt.addWeakObj(sp_this(), &Widget3D_LaserButton::handleNewTextDataSet);

		updateCollisionData(); //make sure this is called after we update position for glitchtext as it uses its tranform
		updateLaserPositions();
	}

	const SA::Transform& Widget3D_LaserButton::getXform()
	{
		return myGlitchText->getXform();
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
		updateLocalAABB();

		myGlitchText->setAnimPlayForward(true); //this will make glyphs invisible until this button is activated.
		myGlitchText->play(false);

		if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem()) //#TODO should never be null... this needs a refactor to return reference
		{
			hashEntry = gameUISystem->getSpatialHash().insert(*this, getOBB());
		}
	}

	void Widget3D_LaserButton::onHoveredhisTick()
	{
		bHoveredThisTick = true; //the tick will consume this
	}

	void Widget3D_LaserButton::onClickedThisTick()
	{
		//click logic is deferred until "tick" so that no weirdness about when the underlying window API's event for click happens
		//eg I don't want this to broadcast while in the middle of the render update.
		bClickedThisTick = true;
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
		accumulatedTickSec += dt_sec;
		myGlitchText->tick(dt_sec);

		if (bHoveredThisTick)
		{
			//consume hover and set color
			setFontColor(hoverColor);
			bHoveredThisTick = false;
		}
		else
		{
			setFontColor(activateColor);
		}

		//passing click last so that any other states will be consumed (eg hover)
		if (bClickedThisTick)
		{
			bClickedThisTick = false;
			lastButtonClickSec = accumulatedTickSec;
			onClickedDelegate.broadcast();
		}

		//colorize the click
		if (accumulatedTickSec < (lastButtonClickSec + clickResetDelaySec))
		{
			setFontColor(clickedColor);
		}

		return true;
	}

	glm::mat4 Widget3D_LaserButton::getModelMatrix() const
	{
		return myGlitchText->getXform().getModelMatrix();
	}

	const std::array<glm::vec4, 8>& Widget3D_LaserButton::getLocalAABB() const
	{
		return localAABB;
	}

	glm::vec3 Widget3D_LaserButton::getWorldLocation() const
	{
		return myGlitchText->getXform().position;
	}

	std::array<glm::vec4, 8> Widget3D_LaserButton::getOBB()
	{
		glm::mat4 xform = myGlitchText->getXform().getModelMatrix();
		std::array<glm::vec4, 8> OBB =
		{
			xform * localAABB[0],
			xform * localAABB[1],
			xform * localAABB[2],
			xform * localAABB[3],
			xform * localAABB[4],
			xform * localAABB[5],
			xform * localAABB[6],
			xform * localAABB[7]
		};
		return OBB;
	}

	bool Widget3D_LaserButton::isHitTestable() const
	{
		return isActive();
	}

	//void Widget3D_LaserButton::changeLaserDelegateSubscription(LaserUIObject& laser, bool bSubscribe)
	//{
	//	//#TODO this may not be needed and can be removed
	//	if (bSubscribe)
	//	{
	//		//add subscription
	//	}
	//	else
	//	{
	//		//remove subscription
	//	}
	//}

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

		//changeLaserDelegateSubscription(*topLaser, true);
		//changeLaserDelegateSubscription(*bottomLaser, true);
		//changeLaserDelegateSubscription(*leftLaser, true);
		//changeLaserDelegateSubscription(*rightLaser, true);

		updateLaserPositions();

		myGlitchText->setAnimPlayForward(true);
		myGlitchText->play(true);
	}

	void Widget3D_LaserButton::onDeactivated()
	{
		LaserUIPool& pool = LaserUIPool::get();

		assert(topLaser && bottomLaser && leftLaser && rightLaser);

		//changeLaserDelegateSubscription(*topLaser, false);
		//changeLaserDelegateSubscription(*bottomLaser, false);
		//changeLaserDelegateSubscription(*leftLaser, false);
		//changeLaserDelegateSubscription(*rightLaser, false);

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
			const Transform& myXform = myGlitchText->getXform();

			float halfWidth = myGlitchText->getWidth() / 2.0f; 
			float halfHeight = myGlitchText->getHeight() / 2.0f;

			halfWidth += widthPaddingFactor * myXform.scale.x;
			halfHeight += heightPaddingFactor * myXform.scale.y;

			vec3 myPos = myXform.getModelMatrix() *  vec4(0, 0, 0, 1);

			GameUIRenderData uiData = {};
			vec3 cUp = uiData.camUp();
			vec3 cRight = uiData.camRight();

			vec3 left = -halfWidth * cRight;
			vec3 right = halfWidth * cRight;
			vec3 top = halfHeight * cUp;
			vec3 bottom = -halfHeight * cUp;

			topLaser->randomizeAnimSpeed(baseLaserAnimSpeed); //prevent interpolation from looking rigid (without this, a line moving could potentially maintain parallel to screen box)
			topLaser->animateStartTo(myPos + right + top);
			topLaser->animateEndTo(myPos + left + top);

			bottomLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			bottomLaser->animateStartTo(myPos + right + bottom);
			bottomLaser->animateEndTo(myPos + left + bottom);

			rightLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			rightLaser->animateStartTo(myPos + right + top);
			rightLaser->animateEndTo(myPos + right + bottom);

			leftLaser->randomizeAnimSpeed(baseLaserAnimSpeed);
			leftLaser->animateStartTo(myPos + left + top);
			leftLaser->animateEndTo(myPos + left + bottom);
		}
	}

	void Widget3D_LaserButton::handleNewTextDataSet()
	{
		updateLocalAABB();
	}

	void Widget3D_LaserButton::updateLocalAABB()
	{
		using namespace glm;

		//local aabb has no transforms applied, just raw size before model matrix is applied
		vec2 size_Unscaled = myGlitchText->getSize_Unscaled();
		
		//currently laser button only supports centered text. To support others we need to get alignment here and do offsets manually for local AABB.
		float halfWidth = size_Unscaled.x / 2.f;
		float halfHeight = size_Unscaled.y / 2.f;
		halfWidth += widthPaddingFactor;
		halfHeight += heightPaddingFactor;
		
		vec2 tl{ -halfWidth, halfHeight };
		vec2 bl{ -halfWidth, -halfHeight };
		vec2 br{ halfWidth, -halfHeight };
		vec2 tr{ halfWidth, halfHeight };


		localAABB[0] = vec4(tl, 0.1f, 1.f);
		localAABB[1] = vec4(bl, 0.1f, 1.f);
		localAABB[2] = vec4(br, 0.1f, 1.f);
		localAABB[3] = vec4(tr, 0.1f, 1.f);

		localAABB[4] = vec4(tl, -0.1f, 1.f);
		localAABB[5] = vec4(bl, -0.1f, 1.f);
		localAABB[6] = vec4(br, -0.1f, 1.f);
		localAABB[7] = vec4(tr, -0.1f, 1.f);
	}

	void Widget3D_LaserButton::updateCollisionData()
	{
		if (hashEntry)
		{
			SpaceArcade::get().getGameUISystem()->getSpatialHash().updateEntry(hashEntry, getOBB());
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "No hash entry for UI button");
			STOP_DEBUGGER_HERE();
		}
	}

	void Widget3D_LaserButton::setFontColor(const glm::vec3& color)
	{
		myGlitchText->setFontColor(color);

		if (topLaser && bottomLaser && leftLaser && rightLaser)
		{
			topLaser->setColorImmediate(color);
			bottomLaser->setColorImmediate(color);
			leftLaser->setColorImmediate(color);
			rightLaser->setColorImmediate(color);
		}
	}

}

