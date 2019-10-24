#pragma once
#include "SABehaviorTree.h"
namespace SA
{
	namespace BehaviorTree
	{
		/////////////////////////////////////////////////////////////////////////////////////
		// Decorator Value Check
		/////////////////////////////////////////////////////////////////////////////////////
		enum class OP
		{
			EQUAL,
			NOT_EQUAL,
			LESS_THAN,
			LESS_THAN_EQUAL,
			GREATER_THAN,
			GREATER_THAN_EQUAL,
		};

		template<typename T>
		class Decorator_Is : public BehaviorTree::Decorator
		{
		public:
			Decorator_Is(const std::string& name, const std::string& memoryKey, OP cmp, const T& value, const sp<NodeBase>& child) :
				BehaviorTree::Decorator(name, child), memoryKey(memoryKey), value(value)
			{
				switch (cmp)
				{
					case OP::LESS_THAN:
						opFunc = [](const T& mem, const T& val) {return mem < val; };
						break;
					case OP::LESS_THAN_EQUAL:
						opFunc = [](const T& mem, const T& val) {return mem <= val; };
						break;
					case OP::GREATER_THAN:
						opFunc = [](const T& mem, const T& val) {return mem > val; };
						break;
					case OP::GREATER_THAN_EQUAL:
						opFunc = [](const T& mem, const T& val) {return mem >= val; };
						break;
					case OP::NOT_EQUAL:
						opFunc = [](const T& mem, const T& val) {return mem != val; };
						break;
					case OP::EQUAL:
					default:
						opFunc = [](const T& mem, const T& val) {return mem == val; };
						break;
				}
			}

			virtual void startBranchConditionCheck()
			{
				Memory& memory = getMemory();
				const T* memoryValue = memory.getReadValueAs<T>(memoryKey);

				//short circuit evaluate
				conditionalResult = (memoryValue != nullptr) && opFunc(*memoryValue, value);
			}
		private:
			virtual void handleNodeAborted() override {}

		private:
			const std::string memoryKey;
			T value;
			std::function<bool(T, T)> opFunc;
		};
	}
}