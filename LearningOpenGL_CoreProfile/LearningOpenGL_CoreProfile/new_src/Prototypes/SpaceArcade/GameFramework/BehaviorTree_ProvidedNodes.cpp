#include "BehaviorTree_ProvidedNodes.h"
#include "SAGameBase.h"

namespace SA
{
	namespace BehaviorTree
	{
		

		void Task_WaitForNextFrame::beginTask()
		{
			evaluationResult = true;
			startFrameNumber = GameBase::get().getFrameNumber();
		}

		bool Task_WaitForNextFrame::isProcessing() const
		{
			bool frameIsStart = GameBase::get().getFrameNumber() == startFrameNumber;
			return Task::isProcessing() || frameIsStart; 
		}

	}
}

