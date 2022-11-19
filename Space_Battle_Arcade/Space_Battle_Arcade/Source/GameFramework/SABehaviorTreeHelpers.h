#pragma once
#include "GameFramework/SABehaviorTree.h"

namespace SA
{
	namespace BehaviorTree
	{
		////////////////////////////////////////////////////////
		// Place holder task node for compiling tree structure
		////////////////////////////////////////////////////////
		class Task_PlaceHolder : public Task
		{
		public:
			Task_PlaceHolder(const std::string& name, bool bEvaluatesToThisValue)
				: Task(name), bEvaluateValue(bEvaluatesToThisValue)
			{
			}
		public:
			virtual void beginTask() override;
		protected:
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		private:
			bool bEvaluateValue = true;
		};


		/////////////////////////////////////////////////////////////////////////////////////
		// Place Holder service
		/////////////////////////////////////////////////////////////////////////////////////
		class Service_PlaceHolder : public Service
		{
		public:
			Service_PlaceHolder(const std::string& name, float updateInterval, bool bLoop,const sp<NodeBase>& child)
				: BehaviorTree::Service(name, updateInterval, true, child),
				updateInterval(updateInterval)
			{}
		private:
			virtual void startService() override;
			virtual void stopService() override {};
			virtual void serviceTick() override;
		private:
			float updateInterval = 0.1f;
			std::string intKeyName = "myInt";
		};

		////////////////////////////////////////////////////////
		// Place Holder Decorator
		////////////////////////////////////////////////////////
		class Decorator_PlaceHolder : public Decorator
		{
		public:
			Decorator_PlaceHolder(const std::string& name, bool bSimulateDecorateSuccess, const sp<NodeBase>& child) :
				BehaviorTree::Decorator(name, child), bSimulateDecorateSuccess(bSimulateDecorateSuccess)
			{}

			virtual void startBranchConditionCheck();
		private:
			virtual void handleNodeAborted() override {}

		private:
			bool bSimulateDecorateSuccess = true;
		};


		/////////////////////////////////////////////////////////////////////////////////////
		// empty task for null trees
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_Empty : public Task
		{
		public:
			Task_Empty(const std::string& name)
				: Task(name)
			{
			}
		public:
			virtual void beginTask() override { evaluationResult = true; }
		protected:
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		private:
		};
	}
}