#include "Widget3D_ProgressBar.h"
#include "../LaserUIPool.h"
#include "../text/DigitalClockFont.h"
#include "../text/GlitchText.h"
#include "../../../../Tools/color_utils.h"

namespace SA
{
	using namespace glm;

	void Widget3D_ProgressBar::postConstruct()
	{
	}

	void Widget3D_ProgressBar::setProgressNormalized(float inValue)
	{
		normalizedValue = glm::clamp<float>(inValue, 0.f, 1.f);
	}

	void Widget3D_ProgressBar::setProgressOnRange(float inValue, float min, float max)
	{
		float top = max - min;
		float adjVal = inValue - min;
		float perc = inValue / top;

		normalizedValue = glm::clamp<float>(perc, 0.f, 1.f);
	}

	void Widget3D_ProgressBar::onActivationChanged(bool bActive)
	{
		if (bActive)
		{
			LaserUIPool& pool = LaserUIPool::get();

			laser_rangeBar = pool.requestLaserObject(laserCollection);
			laser_progressVerticalBar = pool.requestLaserObject(laserCollection);

			laser_boxTop = pool.requestLaserObject(laserCollection);
			laser_boxLeft = pool.requestLaserObject(laserCollection);
			laser_boxBottom = pool.requestLaserObject(laserCollection);
			laser_boxRight = pool.requestLaserObject(laserCollection);

			//force lasers to stick to camera instead of drag
			laser_rangeBar->bForceCameraRelative = true;
			laser_progressVerticalBar->bForceCameraRelative = true;
			laser_boxTop->bForceCameraRelative = true;
			laser_boxLeft->bForceCameraRelative = true;
			laser_boxBottom->bForceCameraRelative = true;
			laser_boxRight->bForceCameraRelative = true;


			laser_rangeBar->		   setColorImmediate(color::red());
			laser_progressVerticalBar->setColorImmediate(color::red());
			laser_boxTop->			   setColorImmediate(color::red());
			laser_boxLeft->			   setColorImmediate(color::red());
			laser_boxBottom->		   setColorImmediate(color::red());
			laser_boxRight->		   setColorImmediate(color::red());


			updateLasers(true);
		}
		else
		{
			laserCollection.clear();//must still clear direct pointers too

			LaserUIPool& pool = LaserUIPool::get();

			pool.releaseLaser(laser_rangeBar);
			pool.releaseLaser(laser_progressVerticalBar);

			pool.releaseLaser(laser_boxTop);
			pool.releaseLaser(laser_boxLeft);
			pool.releaseLaser(laser_boxBottom);
			pool.releaseLaser(laser_boxRight);

			//force lasers to stick to camera instead of drag
			pool.releaseLaser(laser_rangeBar);
			pool.releaseLaser(laser_progressVerticalBar);
			pool.releaseLaser(laser_boxTop);
			pool.releaseLaser(laser_boxLeft);
			pool.releaseLaser(laser_boxBottom);
			pool.releaseLaser(laser_boxRight);

		}
	}

	void Widget3D_ProgressBar::updateLasers(bool bResetAnimation)
	{
		if (isActive() && hasLasers())
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//figure out positions in normalized space, then have xform move them into a correct position
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			float normBarHalfHeight = 0.25f / 2.f;
			float normBarHalfWidth = 1.f / 2.f;
			vec3 verticlOffsetSlanted{ 0, normBarHalfHeight, 0 };

			quat slantQ = glm::angleAxis(barSlantRotation_rad, vec3(0, 0, -1.f));
			verticlOffsetSlanted = slantQ * verticlOffsetSlanted;


			vec3 TL = vec3{ normBarHalfWidth, 0, 0 } +verticlOffsetSlanted;
			vec3 TR = vec3{ -normBarHalfWidth, 0, 0 } +verticlOffsetSlanted;
			vec3 BL = vec3{ normBarHalfWidth, -0, 0 } -verticlOffsetSlanted;
			vec3 BR = vec3{ -normBarHalfWidth, -0, 0 }  -verticlOffsetSlanted;

			float normValueAdjusted = normalizedValue * valueScale_visualAdjustment;
			vec3 toProg = vec3(normValueAdjusted * (normBarHalfWidth*2.f), 0.f, 0.f);
			if (!bLeftToRight) { toProg *= -1.f; }

			vec3 progStart_p = vec3((bLeftToRight ? 1 : -1) * (-normBarHalfWidth), 0, 0); //we start from the opposite direction progress moves up
			vec3 progEnd_p = toProg + progStart_p;
			vec3 progressTop_p = progEnd_p + verticlOffsetSlanted;
			vec3 progressBottom_p = progEnd_p - verticlOffsetSlanted;

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// convert to correct position using transform
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			glm::mat4 modelMatrix = xform.getModelMatrix();
			//todo: can be vectorized and done in single matrix multiplication
			TL = modelMatrix * vec4(TL, 1.f);
			TR = modelMatrix * vec4(TR, 1.f);
			BL = modelMatrix * vec4(BL, 1.f);
			BR = modelMatrix * vec4(BR, 1.f);
			progressTop_p = modelMatrix * vec4(progressTop_p, 1.f);
			progressBottom_p = modelMatrix * vec4(progressBottom_p, 1.f);
			progStart_p = modelMatrix * vec4(progStart_p, 1.f);
			progEnd_p = modelMatrix * vec4(progEnd_p, 1.f);


			//void (LaserUIObject::*fptr)(const glm::vec3& startPoint, const glm::vec3& endPoint) 
			//	= bResetAnimation ? &LaserUIObject::animate : &LaserUIObject::update_NoAnimReset;
			//((*laser_rangeBar).*fptr)(progStart_p, progEnd_p); //very ugly but technically faster, just adding member func

			laser_rangeBar->multiplexedUpdate(bResetAnimation, progStart_p, progEnd_p);
			laser_progressVerticalBar->multiplexedUpdate(bResetAnimation, progressBottom_p, progressTop_p);
			laser_boxLeft->multiplexedUpdate(bResetAnimation, TL, BL);
			laser_boxBottom->multiplexedUpdate(bResetAnimation, BL, BR);
			laser_boxRight->multiplexedUpdate(bResetAnimation, BR, TR);
			laser_boxTop->multiplexedUpdate(bResetAnimation, TR, TL);
		}
	}

	bool Widget3D_ProgressBar::hasLasers()
	{
		for (const sp<LaserUIObject>& laser : laserCollection)
		{
			if (laser == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	void Widget3D_ProgressBar::renderGameUI(GameUIRenderData& renderData)
	{
		updateLasers(false);
		//lasers will automatically be rendered
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Widget3D_TextProgressBar
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Widget3D_TextProgressBar::postConstruct()
	{
		myProgressBar = new_sp<Widget3D_ProgressBar>();

		DigitalClockFont::Data textInit;
		textInit.text = "Unset Progress Bar Text";
		myText = new_sp<GlitchTextFont>(textInit);
		myText->setHorizontalPivot(DigitalClockFont::EHorizontalPivot::CENTER);
		myText->resetAnim();
		myText->play(true);

	}

	//void Widget3D_TextProgressBar::offsetTextPosition()
	//{
	//	Transform textXform = myText->getXform();
	//	... = myProgressBar->getXform();

	//	progressbarCenterPos = ;
	//	textOffset = top ? offsetVec : -offsetVec;
	//	text.setPosition(textOffset);
	//}

	void Widget3D_TextProgressBar::renderGameUI(GameUIRenderData& renderData)
	{
		myProgressBar->renderGameUI(renderData);
		
		if (myProgressBar->isActive())
		{
			if (const RenderData* rd_game = renderData.renderData())
			{
				myText->tick(renderData.dt_sec());
				myText->render(*rd_game);
			}
		}
	}


}

