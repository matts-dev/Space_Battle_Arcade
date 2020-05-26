#pragma once
#include <vector>

#include "Widget3D_ActivatableBase.h"
#include "../Widget3D_LaserButton.h"
#include "../../../../../Tools/DataStructures/MultiDelegate.h"

namespace SA
{
#define GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(FUNC_NAME, field)\
MultiDelegate<>& ##FUNC_NAME(){return field->onClickedDelegate;}

	class Widget3D_MenuScreenBase : public Widget3D_ActivatableBase
	{
		using Parent = Widget3D_ActivatableBase;
	public:
		virtual void tick(float dt_sec);
	protected:
		virtual void postConstruct() override;
		void configureButtonToDefaultBackPosition(Widget3D_LaserButton& Button) const;
	protected:
		/** A container of only the buttons that are enabled; memory is managed by child class*/
		std::vector<Widget3D_LaserButton*> enabledButtons;
	};
}
