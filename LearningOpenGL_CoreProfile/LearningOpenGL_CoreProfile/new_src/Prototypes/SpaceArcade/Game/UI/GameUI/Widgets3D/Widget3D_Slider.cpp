#include <stdio.h>

#include "Widget3D_Slider.h"
#include "../text/GlitchText.h"
#include "../LaserUIPool.h"
#include "../../../../Tools/SAUtilities.h"
#include "../../../../Tools/Geometry/GeometryMath.h"
#include "../../../SpaceArcade.h"
#include "../../../../Tools/PlatformUtils.h"
#include "../../../../GameFramework/SALevel.h"
#include "../../../../GameFramework/SALevelSystem.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../../Tools/DataStructures/MultiDelegate.h"
#include "../../../../GameFramework/TimeManagement/TickGroupManager.h"
#include "../../../../GameFramework/SADebugRenderSystem.h"

namespace SA
{
#define DEBUG_WIDGET3D_SLIDERS 0

	void Widget3D_Slider::postConstruct()
	{
		bRegisterForGameUIRender = true;

		Parent::postConstruct();

		DigitalClockFont::Data textInit;
		textInit.text = "No text set on slider!";
		valueText = new_sp<GlitchTextFont>(textInit);
		titleText = new_sp<GlitchTextFont>(textInit);

		if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem()) //#TODO should never be null... this needs a refactor to return reference
		{
			hashEntry = gameUISystem->getSpatialHash().insert(*this, getOBB());
		}

		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &Widget3D_Slider::handlePreLevelChange);
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			handlePreLevelChange(nullptr, currentLevel);
		}

		valueText->setHorizontalPivot(DigitalClockFont::EHorizontalPivot::LEFT); //by default use left pivot unless changed
		valueText->resetAnim();//make sure text starts in a hidden state that is ready to play forward
		valueText->play(false);//make sure text starts in a hidden state that is ready to play forward

		titleText->setHorizontalPivot(DigitalClockFont::EHorizontalPivot::RIGHT); //by default use left pivot unless changed
		titleText->resetAnim();//make sure text starts in a hidden state that is ready to play forward
		titleText->play(false);//make sure text starts in a hidden state that is ready to play forward		
	}

	void Widget3D_Slider::handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (previousLevel)
		{
			previousLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).removeWeak(sp_this(), &Widget3D_Slider::tick_postCamera);
		}

		if (newCurrentLevel)
		{
			newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).addWeakObj(sp_this(), &Widget3D_Slider::tick_postCamera);
		}
	}

	void Widget3D_Slider::tick_postCamera(float dt_sec)
	{
		using namespace glm;
#if DEBUG_WIDGET3D_SLIDERS 

		if (isActive())
		{
			DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
			debugRenderSystem.renderLine(vec3(localAABB[0]), vec3(localAABB[1]), vec3(1.f,0.f,0.f)); //up (down?)
			debugRenderSystem.renderLine(vec3(localAABB[0]), vec3(localAABB[3]), vec3(1.f,0.f,0.f)); //right
			debugRenderSystem.renderLine(vec3(localAABB[0]), vec3(localAABB[4]), vec3(1.f,0.f,0.f)); //depth

			debugRenderSystem.renderLine(vec3(OBB[0]), vec3(OBB[1]), vec3(0.f, 1.f, 0.f)); //up (down?)
			debugRenderSystem.renderLine(vec3(OBB[0]), vec3(OBB[3]), vec3(0.f, 1.f, 0.f)); //right
			debugRenderSystem.renderLine(vec3(OBB[0]), vec3(OBB[4]), vec3(0.f, 1.f, 0.f)); //depth

			debugRenderSystem.renderSphere(glm::translate(mat4(1.f), worldSliderBoxPos) * glm::scale(mat4(1.f), vec3(0.05f)), vec3(0.f,0.f,1.f)); //visual debug averaged point
		
			debugRenderSystem.renderCube(sliderBoxModelMat, vec3(0.f, 0.5f, 0.5f)); //visual debug the model matrix position and rotation
		}
#endif
		if (isActive())
		{
			if (bHoveredThisTick && !bDragging)
			{
				bHoveredThisTick = false;
				setSliderColor(hoverColor);
			}
			else if (bDragging)
			{
				setSliderColor(dragColor);
			}
			else
			{
				setSliderColor(defaultColor);
			}
		}


		valueText->tick(dt_sec);
		titleText->tick(dt_sec);


	}

	void Widget3D_Slider::renderGameUI(GameUIRenderData& renderData)
	{
		//lasers are automatically rendered from pool

		//render text
		if (const RenderData* rd_game = renderData.renderData())
		{
			valueText->render(*rd_game);
			titleText->render(*rd_game);
		}
	}

	void Widget3D_Slider::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);

		if (bActive)
		{
			onActivated();
		}
		else
		{
			onDeactivated();
		}

	}

	void Widget3D_Slider::setValue(float newValue)
	{
		currentValueAlpha = glm::clamp(newValue, 0.f, 1.f);
	}

	void Widget3D_Slider::setStart(const glm::vec3& start)
	{
		sliderStartPoint = start;
		refresh(); //perhaps should refresh on next tick
	}

	void Widget3D_Slider::setEnd(const glm::vec3& end)
	{
		sliderEndPoint = end;
		refresh();
	}

	std::array<glm::vec4, 8> Widget3D_Slider::getOBB()
	{
		return OBB;
	}

	void Widget3D_Slider::setTitleText(const std::string titleStr)
	{
		titleText->setText(titleStr);
		refresh();
	}

	void Widget3D_Slider::setTitleTextScale(float newScale)
	{
		titleTextScale = newScale;
		refresh();
	}

	//commenting out because switching to system where two texts are used
	//void Widget3D_Slider::setHorizontalPivot(DigitalClockFont::EHorizontalPivot& pivot)
	//{
	//	valueText->setHorizontalPivot(pivot);
	//	refresh();
	//}

	//void Widget3D_Slider::setVerticalPivot(DigitalClockFont::EVerticalPivot& pivot)
	//{
	//	valueText->setVerticalPivot(pivot);
	//	refresh();
	//}

	void Widget3D_Slider::onActivated()
	{
		LaserUIPool& pool = LaserUIPool::get();

		assert(
			!laser_rangeBar
			&& !laser_startEdge
			&& !laser_endEdge
			&& !laser_sliderBoxTop
			&& !laser_sliderBoxLeft
			&& !laser_sliderBoxBottom
			&& !laser_sliderBoxRight
		);

		laser_rangeBar = pool.requestLaserObject();
		laser_startEdge = pool.requestLaserObject();
		laser_endEdge = pool.requestLaserObject();
		laser_sliderBoxTop = pool.requestLaserObject();
		laser_sliderBoxLeft = pool.requestLaserObject();
		laser_sliderBoxBottom = pool.requestLaserObject();
		laser_sliderBoxRight = pool.requestLaserObject();

		updateLaserPositions();

		valueText->setAnimPlayForward(true);
		valueText->play(true);

		titleText->setAnimPlayForward(true);
		titleText->play(true);
	}

	void Widget3D_Slider::refresh()
	{
		updateLaserPositions(/*resetAnimations*/false);
	}

	void Widget3D_Slider::onDeactivated()
	{
		LaserUIPool& pool = LaserUIPool::get();

		assert(
			laser_rangeBar
			&& laser_startEdge
			&& laser_endEdge
			&& laser_sliderBoxTop
			&& laser_sliderBoxLeft
			&& laser_sliderBoxBottom
			&& laser_sliderBoxRight
		);

		//set colors back to default before releasing. (may not be necessary later if I change how release pool works to reset color)
		setSliderColor(defaultColor);

		pool.releaseLaser(laser_rangeBar);
		pool.releaseLaser(laser_startEdge);
		pool.releaseLaser(laser_endEdge);
		pool.releaseLaser(laser_sliderBoxTop);
		pool.releaseLaser(laser_sliderBoxLeft);
		pool.releaseLaser(laser_sliderBoxBottom);
		pool.releaseLaser(laser_sliderBoxRight);

		valueText->setAnimPlayForward(false);
		valueText->resetAnim();

		titleText->setAnimPlayForward(false);
		titleText->resetAnim();
	}

	void Widget3D_Slider::updateLaserPositions(bool bResetAnimations /*= true*/)
{
		using namespace glm;

		if(laser_rangeBar
			&& laser_startEdge
			&& laser_endEdge
			&& laser_sliderBoxTop
			&& laser_sliderBoxLeft
			&& laser_sliderBoxBottom
			&& laser_sliderBoxRight
			&& valueText
		)
		{
			GameUIRenderData uiData = {};
			const vec3 cUp = uiData.camUp();
			const vec3 cRight = uiData.camRight();
			const vec3 cFront = uiData.getHUDData3D().camFront;

			auto configLaser = [this, bResetAnimations](LaserUIObject& laser, glm::vec3 start, glm::vec3 end)
			{
				if (bResetAnimations)
				{
					laser.randomizeAnimSpeed(baseLaserAnimSpeedSec);
					laser.animateStartTo(start);
					laser.animateEndTo(end);
				}
				else
				{
					laser.updateStart_NoAnimReset(start);
					laser.updateEnd_NoAnimReset(end);
				}
			};

			configLaser(*laser_rangeBar, sliderStartPoint, sliderEndPoint);

			//figure out perpendiculars for the edges
			const vec3 horizontal_v = sliderEndPoint - sliderStartPoint;
			const vec3 horizontal_n = glm::normalize(horizontal_v);
			const vec3 sliderWidthVec = sliderBoxWidth * horizontal_n;
			const vec3 sliderWidthVec_half = sliderWidthVec / 2.f;

			const vec3 sliderUp_n = glm::normalize(glm::cross(cFront, horizontal_v)); //may not point in correct handed orthonormal, but will be perpendicular |NAN alert if camera is looking directly along this line
			if (!Utils::anyValueNAN(sliderUp_n))
			{
				vec3 edgeTopDir_v = sliderUp_n * edgeSizeFactor;
				vec3 edgeBottomDir_v = -sliderUp_n * edgeSizeFactor;

				//configure lasers on the edges of the entire range of the slider
				configLaser(*laser_startEdge, sliderStartPoint + edgeTopDir_v, sliderStartPoint + edgeBottomDir_v);
				configLaser(*laser_endEdge, sliderEndPoint + edgeTopDir_v, sliderEndPoint + edgeBottomDir_v);

				////////////////////////////////////////////////////////
				//configure the dragable box on the slider
				////////////////////////////////////////////////////////
				const vec3 sliderPosition = (currentValueAlpha * horizontal_v) + sliderStartPoint;
				const vec3 sliderBox_TL = sliderPosition + edgeTopDir_v - sliderWidthVec_half;
				const vec3 sliderBox_TR = sliderPosition + edgeTopDir_v + sliderWidthVec_half;
				const vec3 sliderBox_BL = sliderPosition + edgeBottomDir_v - sliderWidthVec_half;
				const vec3 sliderBox_BR = sliderPosition + edgeBottomDir_v + sliderWidthVec_half;

				//make box in counter clockwise order, may look jumbled but may look cool :) 
				configLaser(*laser_sliderBoxTop, sliderBox_TR, sliderBox_TL);
				configLaser(*laser_sliderBoxLeft, sliderBox_TL, sliderBox_BL);
				configLaser(*laser_sliderBoxBottom, sliderBox_BL, sliderBox_BR);
				configLaser(*laser_sliderBoxRight, sliderBox_BR, sliderBox_TR);

				////////////////////////////////////////////////////////
				// set up collision info for ui hit-test
				////////////////////////////////////////////////////////
				float halfDepthFactor = 0.1f;
				glm::vec3 frontOffset = cFront * halfDepthFactor;
				OBB[0] = vec4(frontOffset + sliderBox_TL, 1.f);
				OBB[1] = vec4(frontOffset + sliderBox_BL, 1.f);
				OBB[2] = vec4(frontOffset + sliderBox_BR, 1.f);
				OBB[3] = vec4(frontOffset + sliderBox_TR, 1.f);

				OBB[4] = vec4(-frontOffset + sliderBox_TL, 1.f);
				OBB[5] = vec4(-frontOffset + sliderBox_BL, 1.f);
				OBB[6] = vec4(-frontOffset + sliderBox_BR, 1.f);
				OBB[7] = vec4(-frontOffset + sliderBox_TR, 1.f);

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// revsere-construct IMouseInteractable data, kinda sucks but slider is build using points directly and not using typical transforms
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//find average point
				worldSliderBoxPos = vec3(OBB[0] + OBB[1] + OBB[2] + OBB[3] + OBB[4] + OBB[5] + OBB[6] + OBB[7]) / 8.f;//here, slider refers to the thing that is mouse interactable, not the entire widget.

				float sliderBoxHalfWidth = sliderBoxWidth/ 2.f;
				float sliderBoxHalfHeight = edgeSizeFactor; //here, slider refers to the thing that is mouse interactable, not the entire widget.
				localAABB[0] = vec4(-sliderBoxHalfWidth, sliderBoxHalfHeight, halfDepthFactor, 1.f); //TL
				localAABB[1] = vec4(-sliderBoxHalfWidth, -sliderBoxHalfHeight, halfDepthFactor, 1.f); //BL
				localAABB[2] = vec4(sliderBoxHalfWidth, -sliderBoxHalfHeight, halfDepthFactor, 1.f); //BR
				localAABB[3] = vec4(sliderBoxHalfWidth, sliderBoxHalfHeight, halfDepthFactor, 1.f); //TR
				localAABB[4] = vec4(-sliderBoxHalfWidth, sliderBoxHalfHeight, -halfDepthFactor, 1.f); //TL
				localAABB[5] = vec4(-sliderBoxHalfWidth, -sliderBoxHalfHeight, -halfDepthFactor, 1.f); //BL
				localAABB[6] = vec4(sliderBoxHalfWidth, -sliderBoxHalfHeight, -halfDepthFactor, 1.f); //BR
				localAABB[7] = vec4(sliderBoxHalfWidth, sliderBoxHalfHeight, -halfDepthFactor, 1.f); //TR

				//construct a matrix
				sliderBoxModelMat = glm::translate(glm::mat4(1.f), worldSliderBoxPos);

				vec3 sliderBoxRight_n = normalize(sliderBox_BR - sliderBox_BL);
				vec3 sliderBoxUp_n = normalize(sliderBox_TL - sliderBox_BL);
				vec3 sliderBoxDepth_n = normalize(frontOffset);
				mat4 rotMatrix = mat4(vec4(sliderBoxRight_n, 0.f), vec4(sliderBoxUp_n, 0.f), vec4(sliderBoxDepth_n, 0.f), vec4(vec3(0.f), 1.f)); //construct rotation matrix from basis
				sliderBoxModelMat = sliderBoxModelMat * rotMatrix; //scaling is built into the localAABB

				////////////////////////////////////////////////////////
				//update laser text
				////////////////////////////////////////////////////////
				updateValueTextStr();
				updateTextPosition(sliderUp_n, uiData.camQuat());
			}
		}

		updateCollisionData();
	}

	void Widget3D_Slider::updateValueTextStr()
	{
		if (toStringFunc)
		{
			valueText->setText(toStringFunc(getValue()));
		}
	}

	void Widget3D_Slider::updateTextPosition(const glm::vec3& sliderUp_n, const glm::quat& camRot)
	{
		using namespace glm;

		auto updateText = [this, &sliderUp_n, &camRot](const sp<GlitchTextFont>& text, float scale)
		{
			DigitalClockFont::EHorizontalPivot horizontalPivot = text->getHorizontalPivot();
			DigitalClockFont::EVerticalPivot vertPivot = text->getVerticalPivot();

			//apply transform scale so we can calculate accurate width/height
			Transform newTextXform = text->getXform();
			newTextXform.scale = glm::vec3(scale);
			text->setXform(newTextXform);

			glm::vec3 textPosition{ 0.f };
			if (horizontalPivot == DigitalClockFont::EHorizontalPivot::LEFT)
			{
				vec3 slideDir_n = glm::normalize(sliderStartPoint - sliderEndPoint);
				textPosition = sliderStartPoint + (slideDir_n*textPadding.x) + slideDir_n*text->getWidth();
			}
			else if (horizontalPivot == DigitalClockFont::EHorizontalPivot::RIGHT)
			{
				vec3 slideDir_n = glm::normalize(sliderEndPoint - sliderStartPoint);
				textPosition = sliderEndPoint + (slideDir_n * textPadding.x) + (slideDir_n*text->getWidth());
			}
			else
			{
				textPosition = (sliderStartPoint + sliderEndPoint) / 2.f; //take middle point of start and end
			}


			if (vertPivot == DigitalClockFont::EVerticalPivot::TOP)
			{
				textPosition += sliderUp_n * edgeSizeFactor + sliderUp_n*textPadding.y;
			}
			else if (vertPivot == DigitalClockFont::EVerticalPivot::BOTTOM)
			{
				textPosition += -(sliderUp_n * edgeSizeFactor + sliderUp_n * textPadding.y);
			}
			else
			{
				//do nothing, we already took average of points when doing horizontal calculation
			}

			//apply final data
			newTextXform.position = textPosition;
			newTextXform.rotQuat = camRot;
			text->setXform(newTextXform);
		};

		updateText(valueText, valueTextScale);
		updateText(titleText, titleTextScale);
	}

	glm::mat4 Widget3D_Slider::getModelMatrix() const
	{
		return sliderBoxModelMat;
	}

	const std::array<glm::vec4, 8>& Widget3D_Slider::getLocalAABB() const
	{
		return localAABB;
	}

	glm::vec3 Widget3D_Slider::getWorldLocation() const
	{
		return worldSliderBoxPos;
	}

	void Widget3D_Slider::onHoveredhisTick()
	{
		bHoveredThisTick = true;
	}

	void Widget3D_Slider::onClickedThisTick()
	{
		//right now nothing need be done here
	}

	void Widget3D_Slider::onMouseDraggedThisTick(const glm::vec3& worldMousePoint)
	{
		using namespace glm;

		vec3 sliderDir_v = sliderEndPoint - sliderStartPoint;
		vec3 toWorldPoint = worldMousePoint - sliderStartPoint;

		float sliderAlpha = glm::clamp( (scalarProjectAontoB_AutoNormalize(toWorldPoint, sliderDir_v) / glm::length(sliderDir_v)), 0.f, 1.f);
		currentValueAlpha = sliderAlpha;

		updateSliderPosition();

		bDragging = true;

		//setSliderColor(dragColor);
	}

	void Widget3D_Slider::onMouseDraggedReleased()
	{
		bDragging = false;
		bHoveredThisTick = true; //show hovered color immediately instead of on next tick; this will show hover color for a frame if mouse is not on slider.

		//setSliderColor(defaultColor);
	}

	void Widget3D_Slider::updateSliderPosition()
	{
		updateLaserPositions(false);
	}

	void Widget3D_Slider::updateCollisionData()
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

	void Widget3D_Slider::setSliderColor(glm::vec3 color)
	{
		laser_sliderBoxTop->setColorImmediate(color);
		laser_sliderBoxLeft->setColorImmediate(color);
		laser_sliderBoxBottom->setColorImmediate(color);
		laser_sliderBoxRight->setColorImmediate(color);
	}

	std::string Widget3D_Slider_Utils::defaultSliderToString(float value)
	{
		char formatBuffer[128];
		snprintf(formatBuffer, sizeof(formatBuffer), "%.2f", value);
		return std::string(formatBuffer);
	}

}

