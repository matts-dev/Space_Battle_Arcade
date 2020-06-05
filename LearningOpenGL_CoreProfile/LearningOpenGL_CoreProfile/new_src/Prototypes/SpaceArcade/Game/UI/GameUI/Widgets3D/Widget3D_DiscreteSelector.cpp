#include "Widget3D_DiscreteSelector.h"
#include "../text/GlitchText.h"
#include "Widget3D_LaserButton.h"
#include "../../../../GameFramework/SATimeManagementSystem.h"
#include "../../../../GameFramework/SALevel.h"
#include "../../../../GameFramework/SALevelSystem.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../../GameFramework/TimeManagement/TickGroupManager.h"
#include "../../../../Tools/PlatformUtils.h"
#include "../../../../Rendering/Camera/SACameraBase.h"

namespace SA
{
	void Widget3D_DiscreteSelectorBase::postConstruct()
	{
		Parent::postConstruct();

		setRenderWithGameUIDispatch(true);

		//set up text
		DigitalClockFont::Data textInit;
		textInit.text = getStringForItem(currentDiscreteIdx);
		myText = new_sp<GlitchTextFont>(textInit);

		leftButton = new_sp<Widget3D_LaserButton>();
		leftButton->setText("<");
		leftButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_DiscreteSelectorBase::handleLeftClicked);

		rightButton = new_sp<Widget3D_LaserButton>();
		rightButton->setText(">");
		rightButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_DiscreteSelectorBase::handleRightClicked);

		setWorldXform(myText->getXform());

		//#SUGGESTED perhaps needs to update this per level transition; maybe should just put delegate that forwards current level events.
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &Widget3D_DiscreteSelectorBase::handlePreLevelChange);
		
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			handlePreLevelChange(nullptr, currentLevel);
		}

	}

	void Widget3D_DiscreteSelectorBase::renderGameUI(GameUIRenderData& renderData)
	{
		//left/right buttons will render themselves automatically.
		if (const RenderData* game_rd = renderData.renderData())
		{
			myText->render(*game_rd);
		}
	}

	void Widget3D_DiscreteSelectorBase::setIndex(size_t idx)
	{
		if (idx < getNumElements())
		{
			setDiscreteIdx(idx);
		}
		else
		{
			STOP_DEBUGGER_HERE(); //invalid index just set 
		}
	}

	void Widget3D_DiscreteSelectorBase::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);
		if (bActive)
		{
			myText->setAnimPlayForward(true);
			myText->play(true);
		}
		else
		{
			myText->setAnimPlayForward(false);
		}

		leftButton->activate(bActive);
		rightButton->activate(bActive);
	}

	void Widget3D_DiscreteSelectorBase::handleRightClicked()
	{
		size_t numItems = getNumElements();
		size_t newIdx = currentDiscreteIdx;

		newIdx = (newIdx + 1) % numItems ;
		if (numItems == 0)
		{
			newIdx = 0;
		}
		else
		{
			setDiscreteIdx(newIdx);
		}

	}
	void Widget3D_DiscreteSelectorBase::setWorldXform(const Transform& val)
	{ 
		worldXform = val; 
		myText->setXform(worldXform);
		updateButtonLayout();
	}

	void Widget3D_DiscreteSelectorBase::handleLeftClicked()
	{
		size_t numItems = getNumElements();
		size_t newIdx = currentDiscreteIdx;
		if (newIdx == 0)
		{
			newIdx = numItems - 1;
		}
		else
		{
			newIdx = (newIdx - 1);
		}

		if (numItems == 0)
		{
			newIdx = 0;
		}
		else
		{
			setDiscreteIdx(newIdx);
		}
	}

	void Widget3D_DiscreteSelectorBase::handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (previousLevel)
		{
			newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).removeWeak(sp_this(), &Widget3D_DiscreteSelectorBase::tick);
		}

		if (newCurrentLevel)
		{
			newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).addWeakObj(sp_this(), &Widget3D_DiscreteSelectorBase::tick);
		}
	}

	void Widget3D_DiscreteSelectorBase::tick(float dt_sec)
	{
		//Parent::tick(dt_sec);

		myText->tick(dt_sec);
		leftButton->tick(dt_sec);
		rightButton->tick(dt_sec);
	}

	void Widget3D_DiscreteSelectorBase::setDiscreteIdx(size_t idx)
	{
		currentDiscreteIdx = idx;

		updateText();
		updateButtonLayout();
	}

	void Widget3D_DiscreteSelectorBase::updateText()
	{
		myText->setText(getStringForItem(currentDiscreteIdx));
	}

	void Widget3D_DiscreteSelectorBase::updateButtonLayout()
	{
		using namespace glm;

		GameUIRenderData ui_rd = {};
		if (CameraBase* cam = ui_rd.camera())
		{
			const Transform& textTransform = myText->getXform();
			vec3 pos = textTransform.position;
			vec3 offset = (myText->getWidth() / 2.f) * cam->getRight();

			Transform buttonXform = textTransform;

			//this is costly, but we need to update button size before using its size to calculate its offset location
			leftButton->setXform(buttonXform);
			rightButton->setXform(buttonXform);

			buttonXform.position = pos + -offset + (leftButton->getPaddedSize().x * -cam->getRight()); //perhaps should create a padded size and use that instead
			leftButton->setXform(buttonXform);

			buttonXform.position = pos + offset + (rightButton->getPaddedSize().x * cam->getRight());
			rightButton->setXform(buttonXform);
		}else { STOP_DEBUGGER_HERE(); }

	}

	void Widget3D_DiscreteSelectorBase::refreshVisuals()
	{
		updateText();
		updateButtonLayout();
	}

}
