#include "SABehaviorTreeHelpers.h"
#include "SALog.h"

namespace SA
{
	namespace BehaviorTree
	{
		////////////////////////////////////////////////////////
		// task place holder
		////////////////////////////////////////////////////////
		void Task_PlaceHolder::beginTask()
		{
			log("Task_PlaceHolder", LogLevel::LOG_WARNING, "TEMP BEHAVIOR TREE TASK IN USE");
			evaluationResult = bEvaluateValue;
		}

		////////////////////////////////////////////////////////
		// service place holder
		////////////////////////////////////////////////////////s
		void Service_PlaceHolder::serviceTick()
		{
			log("Service_PlaceHolder", LogLevel::LOG_WARNING, "TEMP BEHAVIOR TREE SERVICE TICK");
		}

		void Service_PlaceHolder::startService()
		{
			log("Service_PlaceHolder", LogLevel::LOG_WARNING, "TEMP BEHAVIOR TREE SERVICE START");
		}

		////////////////////////////////////////////////////////
		// decorator place holder
		////////////////////////////////////////////////////////
		void Decorator_PlaceHolder::startBranchConditionCheck()
		{
			log("Decorator_PlaceHolder", LogLevel::LOG_WARNING, "TEMP BEHAVIOR TREE DECORATOR EVALUATED");
			conditionalResult = bSimulateDecorateSuccess;
		}
	}
}
