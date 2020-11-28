#pragma once
#include "../Widget3D_Base.h"

namespace SA 
{
	class Widget3D_ActivatableBase : public Widget3D_Base
	{
	public:
		void activate(bool bEnable)
		{
			if (bActive != bEnable)
			{
				bActive = bEnable;
				onActivationChanged(bActive);
			}
		}
		bool isActive() const { return bActive; }
	protected:
		virtual void onActivationChanged(bool bActive) {}
	private:
		bool bActive = false;
	};

}