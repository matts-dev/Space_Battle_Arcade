#pragma once
#include <string>

#include "MainMenuScreens/Widget3D_ActivatableBase.h"
#include "../../../../Tools/DataStructures/MultiDelegate.h"


namespace SA
{
	class GlitchTextFont;
	class Widget3D_LaserButton;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// base class for discrete selector that does not require template information
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_DiscreteSelectorBase : public Widget3D_ActivatableBase
	{
		using Parent = Widget3D_ActivatableBase;
	protected:
		virtual void postConstruct() override;
	public:
		const Transform& getWorldXform() const { return worldXform; }
		void setWorldXform(const Transform& val);
		virtual void renderGameUI(GameUIRenderData& renderData) override;
		void setIndex(size_t idx);
	protected:
		virtual size_t getNumElements() = 0;
		virtual std::string getStringForItem(size_t idx) = 0;
		virtual void onActivationChanged(bool bActive) override;
		virtual void onValueChanged() = 0;
	protected:
		void updateText();
		void updateButtonLayout();
		void refreshVisuals();
		inline size_t getCurrentIdx() const { return currentDiscreteIdx; }
	private:
		void handleRightClicked();
		void handleLeftClicked();
		void handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
		void tick(float dt_sec);
		void setDiscreteIdx(size_t idx);
	private:
		size_t currentDiscreteIdx = 0;
		Transform worldXform;
		sp<GlitchTextFont> myText = nullptr;
		sp<Widget3D_LaserButton> leftButton = nullptr;
		sp<Widget3D_LaserButton> rightButton = nullptr;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Select between discrete states stored in the array 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	class Widget3D_DiscreteSelector : public Widget3D_DiscreteSelectorBase
	{
		using ToStringFunc = std::function<std::string(const T&)>;
	public:
		void setSelectables(const std::vector<T>& toCopy);
		void setToString(const ToStringFunc& toStringCallback);
		virtual size_t getNumElements() override;
		virtual std::string getStringForItem(size_t idx) override;
		const T& getValue();
	protected:
		virtual void onValueChanged() override;
	public:
		MultiDelegate<const T&> onValueChangedDelegate;
	private:
		ToStringFunc valueToStringFunction;
		std::vector<T> selectables;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// template implementation 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void SA::Widget3D_DiscreteSelector<T>::onValueChanged()
	{
		//alert world that value changed
		onValueChangedDelegate.broadcast(getValue());
	}


	template <typename T>
	const T& SA::Widget3D_DiscreteSelector<T>::getValue()
	{
		size_t currentIdx = getCurrentIdx();

		//make sure we have valid data
		assert(selectables.size() > 0 && currentIdx < selectables.size());

		return selectables[currentIdx];
	}

	template <typename T>
	void SA::Widget3D_DiscreteSelector<T>::setSelectables(const std::vector<T>& toCopy)
	{
		selectables = toCopy;
		refreshVisuals();
	}

	template <typename T>
	std::string SA::Widget3D_DiscreteSelector<T>::getStringForItem(size_t idx)
	{
		if (idx < selectables.size() && valueToStringFunction)
		{
			return valueToStringFunction(selectables[idx]);
		}
		else
		{
			return "invalid index";
		}
	}

	template <typename T>
	size_t SA::Widget3D_DiscreteSelector<T>::getNumElements()
	{
		return selectables.size();
	}

	template <typename T>
	void SA::Widget3D_DiscreteSelector<T>::setToString(const ToStringFunc& toStringCallback)
	{
		valueToStringFunction = toStringCallback;
		refreshVisuals();
	}


	////////////////////////////////////////////////////////
	// Some useful to-strings
	////////////////////////////////////////////////////////
	namespace DiscreteToStringDefaults
	{
		template <typename INT_TYPE>
		std::string intToString(INT_TYPE value)
		{
			return std::to_string(value);
		}
	}

}