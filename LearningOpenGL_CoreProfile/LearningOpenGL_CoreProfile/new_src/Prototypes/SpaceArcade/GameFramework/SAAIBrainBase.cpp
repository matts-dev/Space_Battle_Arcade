#include "SAAIBrainBase.h"
#include "SALog.h"
#include <assert.h>
#include "..\Tools\DataStructures\MultiDelegate.h"
#include "SALevelSystem.h"
#include "SAGameBase.h"
#include "SALevel.h"

namespace SA
{
	bool AIBrain::awaken()
	{
		bActive = onAwaken();
		return bActive;
	}

	void AIBrain::sleep()
	{
		onSleep();
		bActive = false;
	}

	namespace BehaviorTree
	{
		/////////////////////////////////////////////////////////////////////////////////////
		// Behavior tree
		/////////////////////////////////////////////////////////////////////////////////////
		Tree::Tree(const std::string& name, const sp<NodeBase>& root, std::vector<std::pair<std::string, sp<GameEntity>>> initializedMemory)
			: NodeBase(name), root(root)
		{
			memory = new_sp<Memory>();
			for (const auto& kv_pair : initializedMemory)
			{
				memory->replaceValue(kv_pair.first, kv_pair.second);
			}

			//tree structure is now set; prioritize tree nodes (eg for aborting of lower priority sub-trees)
			uint32_t startNodePriority = 0;
			possessNodes(root, startNodePriority);
			numNodes = startNodePriority;

		}

		void Tree::start()
		{
			if (!bExecutingTree)
			{
				bExecutingTree = true;
				executionStack.clear();
				executionStack.push_back(this/*root.get()*/);
			}
		}

		void Tree::stop()
		{
			if (bExecutingTree)
			{
				bExecutingTree = false;
				resumeData.reset();

				//#TODO clean up and stop tree
			}
		}

		void Tree::possessNodes(const sp<NodeBase>& node, uint32_t& currentPriority)
		{
			node->priority = currentPriority++;
			node->owningTree = this;
			node->assignedMemory = this->memory.get();
			for (const sp<NodeBase>& childNode : node->children)
			{
				possessNodes(childNode, currentPriority);
			}
			node->notifyTreeEstablished();
		}

		/*
			Tip Debugging this method: I recommend putting the currentState and CurrentNode->nodeName in watch window
			This will give a clear picture of the what the state machine is doing.
		*/
		void Tree::tick(float delta_sec)
		{
			/* 
			Design considerations:
				-only a single tree walk should be permitted per tick; otherwise a tree can get into an inifite loop.
					-implementing a 1-tree-walk-per-frame
						-cannot use the starting node because that node may no be revisited depending on conditions
						-cannot use node priority because the tree may not have a state to reach that priority on second pass
						-using node visit count lets avoid tree state entirely in determining how far we can walk this frame; 
							- we can set this equal to the number of nodes in the tree to limit the tree to a walk of the entire tree.
					-optimizations
						-need a way to check if same node keeps being visited, but nothing is happening -- if so we don't want to visit the same node the over and over (until we hit number of nodes in trees) we want to just stop and let tick continue
			
			*/
			if (bExecutingTree)
			{
				if (NodeBase* currentNode = executionStack.back())
				{
					ExecutionState currentState = ExecutionState::STARTING;
					bool bNodeCompletedSuccessfully = false;

					if (resumeData.has_value())
					{
						currentState = resumeData->state;
						bNodeCompletedSuccessfully = resumeData->bChildResult;
					}

					uint32_t nodesVisited = 0;
					bool bReachedMaxNodesThisTick = false;

					while(!currentNode->isProcessing() && !bReachedMaxNodesThisTick)
					{
						//USE PREVIOUS STATE
						if (currentState == ExecutionState::POPPED_CHILD) 
						{ 
							currentNode->notifyCurrentChildResult(bNodeCompletedSuccessfully);
						} 
						else if (currentState == ExecutionState::PUSHED_CHILD)
						{
							currentNode->evaluate();
						}

						//PREPARE NEXT STATE
						if (currentNode->hasPendingChildren())
						{
							if (NodeBase* child = currentNode->getNextChild())
							{
								executionStack.push_back(child);
								currentState = ExecutionState::PUSHED_CHILD;
								currentNode = child;
								nodesVisited++;
							}
							else { log("behavior_tree", LogLevel::LOG_ERROR, "node requested child evaluation but child not available"); }
						}
						else if (currentNode->resultReady())
						{
							bNodeCompletedSuccessfully = currentNode->result();
							currentNode->resetNode();
							if (executionStack.size() > 1) { executionStack.pop_back(); }
							else { nodesVisited++; /* we're at root, count this a new visited node */}
							bReachedMaxNodesThisTick = nodesVisited >= numNodes;
							currentState = ExecutionState::POPPED_CHILD;
							currentNode = executionStack.back();
						}
						else 
						{
							currentState = ExecutionState::CHILD_EXECUTING;
						}
					}
					
					resumeData = ResumeStateData{};
					resumeData->state = currentState;
					resumeData->bChildResult = bNodeCompletedSuccessfully;

				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Single child node helper
		/////////////////////////////////////////////////////////////////////////////////////
		void SingleChildNode::notifyCurrentChildResult(bool childResult)
		{
			bChildExecutionResult = childResult;
			bChildReturned = true;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Multi child node helper
		/////////////////////////////////////////////////////////////////////////////////////
		MultiChildNode::MultiChildNode(const std::string& name, const std::vector<sp<NodeBase>>& children) : NodeBase(name)
		{
			this->children = children;
			if (children.size() == 0)
			{
				//efficient tree structures meaning not bounds checking child index
				throw std::runtime_error("invalid multichild node; no children passed. This will corrupt efficient tree structures and cannot be permitted.");
			}
			resetMultiNode();
		}

		void MultiChildNode::notifyCurrentChildResult(bool childResult)
		{
			if (childIdx < childResults.size())
			{
				childResults[childIdx] = childResult;
				childHasReturned[childIdx] = true;
			}
		}

		bool MultiChildNode::tryIncrementChild()
		{
			if (childIdx < children.size() - 1)
			{
				childIdx++;
				return true;
			}
			else
			{
				return false;
			}
		}

		void MultiChildNode::resetMultiNode()
		{
			/* non virtual; intended to be safe to call within constructor; be careful adding virtual calls here*/
			childIdx = 0;
			childResults = std::vector<bool>(children.size(), false);
			childHasReturned = std::vector<bool>(children.size(), false);
		}

		NodeBase* MultiChildNode::getNextChild()
		{
			return children[childIdx].get();
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Selector node
		/////////////////////////////////////////////////////////////////////////////////////
		bool Selector::hasPendingChildren() const
		{
			size_t childIdx = getCurrentChildIdx();

			if (childHasReturned[childIdx])
			{
				if (childResults[childIdx])
				{
					//current child succeeded! select this one
					return false;
				}
				else
				{
					//test if we tried all our children.
					return childIdx != (children.size() - 1);
				}
			}
			else
			{
				//covers case where: we just incremented and this gets called again; child hasn't returned so getNextChild should still return
				return true;
			}
		}

		bool Selector::resultReady() const
		{
			if (!hasPendingChildren())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		NodeBase* Selector::getNextChild()
		{
			size_t childIdx = getCurrentChildIdx();
			if (childHasReturned[childIdx] && !childResults[childIdx])
			{
				//current child has failed, try the next one.
				tryIncrementChild();
			}

			return MultiChildNode::getNextChild();
		}

		bool Selector::result() const
		{
			//state machine should never call this when the results are not ready.
			return childResults[getCurrentChildIdx()];
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Sequence node
		/////////////////////////////////////////////////////////////////////////////////////
		bool Sequence::hasPendingChildren() const
		{
			size_t childIdx = getCurrentChildIdx();

			if (childHasReturned[childIdx])
			{
				if (!childResults[childIdx])
				{
					//this child failed, the whole sequence has failed.
					return false;
				}
				else
				{
					//is there more children in this sequence?
					return childIdx != (children.size() - 1);
				}
			}
			else
			{
				//child is still processing. this shouldn't be called by state machine but may be called internally to class.
				return true;
			}
		}

		bool Sequence::resultReady() const
		{
			if (!hasPendingChildren())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		SA::BehaviorTree::NodeBase* Sequence::getNextChild()
		{
			size_t childIdx = getCurrentChildIdx();
			if (childHasReturned[childIdx] && childResults[childIdx])
			{
				//current child has succeeded, try the next one.
				//note: if the current child failed, then the state machine won't be calling this method.
				tryIncrementChild();
			}

			return MultiChildNode::getNextChild();
		}

		bool Sequence::result() const
		{
			//state machine should never call this when the results are not ready.
			//state machine may call this on non-last child, if the sequence failed before the end
			return childResults[getCurrentChildIdx()];
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Tasks
		/////////////////////////////////////////////////////////////////////////////////////
		void Task::resetNode()
		{
			NodeBase::resetNode();
			evaluationResult.reset();
			bStartedTask = false;
		}

		bool Task::isProcessing() const
		{
			return !evaluationResult.has_value() && bStartedTask;
		}

		bool Task::resultReady() const
		{
			return !isProcessing();
		}

		bool Task::result() const
		{
			//state machine should never call this if there isn't yet a result.
			assert(evaluationResult.has_value());
			return evaluationResult.value_or(false);
		}

		void Task::evaluate()
		{
			//don't require the user to remember to update bStartedTask.  Just require the user fill the evaluation result when done.
			//having a bool bStarted prevents the "is processing" from being true before any evaluation has started. 
			bStartedTask = true;
			beginTask();
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Service
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		Service::Service(const std::string& name, float tickSecs, bool bLoop, const sp<NodeBase>& child) 
			: SingleChildNode(name, child), tickSecs(tickSecs), bLoop(bLoop)
		{
		}

		void Service::resetNode()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();

			//remove binding from delegate to be extra sure this will not be called
			timerDelegate->removeWeak(sp_this(), &Service::serviceTick);
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				//invariant that levels must have timer manager
				currentLevel->getWorldTimeManager()->removeTimer(timerDelegate);
			}
			else
			{
				log("BehaviorTree::Service", LogLevel::LOG_WARNING, "failure: no level to use for enforce service tick");
			}

			stopService();
		}

		void Service::evaluate()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();

			timerDelegate->addWeakObj(sp_this(), &Service::serviceTick);
			const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();
			const sp<TimeManager>& timerManager = currentLevel ? currentLevel->getWorldTimeManager() : nullptr;
			if (currentLevel && timerManager)
			{
				timerManager->createTimer(timerDelegate, tickSecs, bLoop);
			}
			else { log("BehaviorTree::Service", LogLevel::LOG_WARNING, "failure: no level/timermanager to use for enforce service tick"); }

			startService();

			if (bExecuteOnStart)
			{
				serviceTick();
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// decorators
		/////////////////////////////////////////////////////////////////////////////////////
		bool Decorator::isProcessing() const
		{
			return bStartedDecoratorEvaluation && !conditionalResult.has_value();
		}

		void Decorator::evaluate()
		{
			bStartedDecoratorEvaluation = true;
			startBranchConditionCheck();
		}

		bool Decorator::hasPendingChildren() const
		{
			assert(bStartedDecoratorEvaluation);
			return SingleChildNode::hasPendingChildren() && conditionalResult.has_value() && conditionalResult.value();
		}

		void Decorator::resetNode()
		{
			SingleChildNode::resetNode();
			conditionalResult.reset();
			bStartedDecoratorEvaluation = false;
		}

	}
}

