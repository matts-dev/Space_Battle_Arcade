#pragma once
#include "GameFramework/SABehaviorTree.h"
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

		enum class AbortPreference
		{
			NO_ABORT,
			ABORT_ON_MODIFY,
			ABORT_ON_REPLACE,
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
		protected:
			virtual void startBranchConditionCheck() override
			{
				Memory& memory = getMemory();
				const T* memoryValue = memory.getReadValueAs<T>(memoryKey);

				//short circuit evaluate
				conditionalResult = doConditionCheck(memoryValue, &value);
			}

			/** Separated this from startBranchConditionCheck so subclasses can use it. */
			inline bool doConditionCheck(const T* runtimeValue, const T* nodePropertyValue) const
			{
				return (runtimeValue != nullptr) && opFunc(*runtimeValue, *nodePropertyValue);
			}

		private:
			virtual void handleNodeAborted() override {}

		protected: //node properties
			const std::string memoryKey;
			const T value;
		private: //node properties
			std::function<bool(T, T)> opFunc;
			
		};

		/** Separate from Decorator_Is because caches start value. Which is not great for memory values that
			are not POD*/
		template<typename T>
		class Decorator_Aborting_Is : public Decorator_Is<T>
		{
		public:
			Decorator_Aborting_Is(const std::string& name, const std::string& memoryKey, OP cmp, const T& value, AbortPreference abortPref, const sp<NodeBase>& child)
				: Decorator_Is<T>(name, memoryKey, cmp, value, child), abortPref(abortPref)
			{
			}
		protected:

			virtual void startBranchConditionCheck() override
			{
				Decorator_Is<T>::startBranchConditionCheck();
				if (*Decorator::conditionalResult)
				{
					Memory& memory = NodeBase::getMemory();
					bSubscribedDelegates = true;
					if (abortPref == AbortPreference::ABORT_ON_MODIFY)
					{
						memory.getModifiedDelegate(this->memoryKey).template addStrongObj<Decorator_Aborting_Is<T>>(sp_this(), &Decorator_Aborting_Is<T>::handleValueModified);
					}
					else if (abortPref == AbortPreference::ABORT_ON_REPLACE)
					{
						memory.getReplacedDelegate(this->memoryKey).template addStrongObj<Decorator_Aborting_Is<T>>(sp_this(), &Decorator_Aborting_Is<T>::handleValueReplaced);
					}
					else if (abortPref == AbortPreference::NO_ABORT)
					{
						//do nothing
					}
				}
			}

			virtual void resetNode() override
			{
				if (bSubscribedDelegates)
				{
					Memory& memory = NodeBase::getMemory();
					if (abortPref == AbortPreference::ABORT_ON_MODIFY)
					{
						//memory.getModifiedDelegate(this->memoryKey).removeStrong(sp_this(), &Decorator_Aborting_Is<T>::handleValueModified);
					}
					else if (abortPref == AbortPreference::ABORT_ON_REPLACE)
					{
						//memory.getReplacedDelegate(this->memoryKey).removeStrong(sp_this(), &Decorator_Aborting_Is<T>::handleValueReplaced);
					}
					else if (abortPref == AbortPreference::NO_ABORT)
					{
						//do nothing
					}
				}
				bSubscribedDelegates = false;
				Decorator_Is<T>::resetNode();
			}
		private:
			void abortIfValueChanged()
			{
				Memory& memory = NodeBase::getMemory();
				const T* newValue = memory.getReadValueAs<T>(this->memoryKey);

				if (!this->doConditionCheck(newValue, &this->value))
				{
					this->abortTree(Decorator::AbortType::ChildTree);
				}
			}

			void handleValueReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
			{
				//it is not safe to cast GameEntity to PrimitiveWrapper<T> because it may have been replaced with a different type
				abortIfValueChanged();
			}

			void handleValueModified(const std::string& key, const GameEntity* oldValue)
			{
				//it is not safe to cast GameEntity to PrimitiveWrapper<T> because it may have been replaced with a different type (this gets called if replace is called)
				abortIfValueChanged();
			}

		private:
			bool bSubscribedDelegates;
		private: //node properties
			const AbortPreference abortPref;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_SleepThisFrame Sleeps the behavior tree for a single frame
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Task_WaitForNextFrame : public Task
		{
		public:
			Task_WaitForNextFrame(const std::string& nodeName): Task(nodeName)
			{}
		protected:
			virtual void beginTask() override;
			virtual bool isProcessing() const override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		private:
			uint64_t startFrameNumber = 0;
		};		
	}
}