#include "GameFramework/SABehaviorTree.h"

#include "GameFramework/SALog.h"
#include <assert.h>
#include "Tools/DataStructures/MultiDelegate.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALevel.h"
#include <map>
#include "SARandomNumberGenerationSystem.h"



namespace SA
{

	namespace BehaviorTree
	{
		Tree* volatile targetDebugTree = nullptr;

		/////////////////////////////////////////////////////////////////////////////////////
		// Behavior tree
		/////////////////////////////////////////////////////////////////////////////////////
		Tree::Tree(const std::string& name, const sp<NodeBase>& root, std::vector<std::pair<std::string, sp<GameEntity>>> initializedMemory)
			: NodeBase(name), root(root)
		{
			memory = new_sp<Memory>();
			this->assignedMemory = this->memory.get(); //make sure that if this is accessed as a node, it returns correct memory.
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
				//clear execution stack using abort feature.
				abort(0);
				ExecutionState mockState;
				NodeBase* mockCurrentNode = executionStack.back();
				processAborts(mockCurrentNode, mockState);

				bExecutingTree = false;
				resumeData.reset();
				abortPriority.reset();
			}
		}

		void Tree::abort(uint32_t priority, NodeBase* inAbortInstigator /*= nullptr*/)
		{
			//aborting is done in state machine tick. That will clear the optional.
			// design considerations
			//		-the outside world probably shouldn't be able to query current abort priority; that will allow some needlessly complex logic like "stop aborting if someone else decided to abort"; those kinds of things should be addressed with the tree structure.
			//		-do not stomp previous aborts that have a higher priority (lower number is higher in priority)
			//		-abort instigator provided for debugging, only updates if priority is greater

			if (abortPriority.has_value()
				&& abortPriority.value() <= priority)	//short-circuit evaluation
			{
				return;									//already aborting a higher priority, do not stomp this value.
			}

			abortPriority = priority;
			abortInstigator = inAbortInstigator;
		}

		uint32_t Tree::getCurrentPriority()
		{
			assert(executionStack.size() > 0);
			return executionStack.back()->getPriority();
		}

		SA::BehaviorTree::Memory& Tree::getMemory() const
		{
			// This overloads the node version of the method to make it public. 
			// even if node version is made public, This being on the tree makes the API more clear as someone only looking 
			// through the behavior tree class for the first time, they don't need to know Nodes have a function to get memory.
			assert(memory.get() != nullptr);
			return *memory;
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
			frame_dt_sec = delta_sec;
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

					//process any aborts before we enter loop; covers case where aborts happened externally to tree execution.
					processAborts(currentNode, currentState);

					while (!currentNode->isProcessing() && !bReachedMaxNodesThisTick)
					{
						if constexpr (LOG_TREE_STATE) { treeLog(currentState, currentNode, nodesVisited); }

						//USE PREVIOUS STATE
						if (currentState == ExecutionState::POPPED_CHILD)	//DEBUGGING: breakpoint here and watch "currentNode->nodeName" and "currentState" and continue to step through tree
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
								child->bOnExecutionStack = true; //#suggested it may be worth-while to have a "onPushedToExecutionStack" and "onPopedFromStack" methods that do this and other things
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
							else { nodesVisited++; /* we're at root, count this a new visited node */ }
							bReachedMaxNodesThisTick = nodesVisited >= numNodes;
							currentState = ExecutionState::POPPED_CHILD;
							currentNode = executionStack.back();
						}
						else
						{
							currentState = ExecutionState::CHILD_EXECUTING;
						}

						//ABORT CHECK; this needs happen within loop because a "processing" node will sleep the loop.
						processAborts(currentNode, currentState);
					}

					resumeData = ResumeStateData{};
					resumeData->state = currentState;
					resumeData->bChildResult = bNodeCompletedSuccessfully;

				}
			}
		}

		void Tree::processAborts(NodeBase*& inOut_CurrentNode, ExecutionState& inOut_currentState)
		{
			assert(inOut_CurrentNode != nullptr);

			if (abortPriority.has_value())
			{
				if constexpr (LOG_TREE_STATE) { treeLogAbortInstigator(); }

				while (inOut_CurrentNode->getPriority() >= abortPriority.value()
					&& executionStack.size() > 1)
				{
					if constexpr (LOG_TREE_STATE) { treeLog(ExecutionState::ABORT, inOut_CurrentNode, 0); }

					inOut_CurrentNode->handleNodeAborted();
					inOut_CurrentNode->resetNode();
					if (executionStack.size() > 1) { executionStack.pop_back(); }
					inOut_CurrentNode = executionStack.back();
				}
				abortPriority.reset();
				abortInstigator = nullptr;

				inOut_CurrentNode->resetNode();
				inOut_currentState = ExecutionState::PUSHED_CHILD;
			}
		}

		void Tree::treeLog(ExecutionState state, NodeBase* currentNode, uint32_t nodesVisited)
		{
			if (targetDebugTree && this != targetDebugTree)
			{
				return; //filter is present and this is the wrong instance; abort.
			}

			std::string stateString;
			switch (state)
			{
				case ExecutionState::STARTING:
					stateString = "> STARTING";
					break;
				case ExecutionState::CHILD_EXECUTING:
					stateString = "< EXECUTING";
					break;
				case ExecutionState::POPPED_CHILD:
					stateString = "- POP_TO_PARENT";
					break;
				case ExecutionState::PUSHED_CHILD:
					stateString = "+ PUSHED_CHILD";
					break;
				case ExecutionState::ABORT:
					stateString = "* ABORT";
					break;
				default:
					stateString = "? OTHER";
			}

			std::string treeInstance = std::to_string((uint64_t)this);
			std::string numNodesStr = std::to_string(nodesVisited);
			std::string nodeName = currentNode ? currentNode->nodeName : std::string("null-node");

			std::string logStr = treeInstance + "\t state: " + stateString + "\t" + nodeName + "\t\t numNodes:" + numNodesStr;

			log("BehaviorTreeLog", LogLevel::LOG, logStr.c_str());

		}

		void Tree::treeLogAbortInstigator()
		{
			if (targetDebugTree && this != targetDebugTree)
			{
				return; //filter is present and this is the wrong instance; abort.
			}

			std::string treeInstance = std::to_string((uint64_t)this);

			std::string logStr = treeInstance + "\t state: * ABORTING - ";
			std::string instigatorName = (abortInstigator != nullptr) ? abortInstigator->getName() : std::string("abort instigator unspecified");
			logStr = logStr + "INSTIGATOR: " + instigatorName;

			log("BehaviorTreeLog", LogLevel::LOG, logStr.c_str());
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

			childResults = std::vector<bool>(children.size(), false);
			childHasReturned = std::vector<bool>(children.size(), false);

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
			NodeBase::resetNode();
			childIdx = 0;

			assert(childResults.size() == childHasReturned.size());
			for (size_t childIdx = 0; childIdx < childResults.size(); ++childIdx)
			{
				childResults[childIdx] = false;
				childHasReturned[childIdx] = false;
			}
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
			return !hasPendingChildren();
			//if (!hasPendingChildren())
			//{
			//	return true;
			//}
			//else
			//{
			//	return false;
			//}
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
			taskCleanup();

			NodeBase::resetNode();
			evaluationResult.reset();
			bStartedTask = false;
		}

		bool Task::isProcessing() const
		{
			return !evaluationResult.has_value() && bStartedTask;
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
			NodeBase::resetNode();
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

		void Decorator::abortTree(AbortType abortType)
		{
			Tree& tree = getTree();

			if (abortType == AbortType::ChildTree || abortType == AbortType::ChildAndLowerPriortyTrees)
			{
				if (isOnExecutionStack())
				{
					tree.abort(getPriority(), this);
					return; //don't need to execute code to abort lower priorities because we're already aborting them
				}
			}
			if (abortType == AbortType::LowerPriortyTrees || abortType == AbortType::ChildAndLowerPriortyTrees)
			{
				bool bIsOnChildTree = isOnExecutionStack();
				bool bIsOnLowerPriorityTree = tree.getCurrentPriority() > getPriority();
				if (!bIsOnChildTree && bIsOnLowerPriorityTree)
				{
					//checking if this node is on the execution path will prevent
					//it from aborting its children, technically priority + 1 is its first child
					tree.abort(getPriority() + 1, this);
				}
			}
		}

		void Decorator::resetNode()
		{
			SingleChildNode::resetNode();
			conditionalResult.reset();
			bStartedDecoratorEvaluation = false;
		}


		/////////////////////////////////////////////////////////////////////////////////////
		// Loop node
		/////////////////////////////////////////////////////////////////////////////////////
		bool Loop::resultReady() const
		{
			if (bInifinteLoop)
			{
				//use abort decorators to get out of infinte loops
				return false;
			}
			else
			{
				return currentLoop >= numLoops;
			}
		}

		void Loop::notifyCurrentChildResult(bool childResult)
		{
			currentLoop += 1;
			SingleChildNode::notifyCurrentChildResult(childResult);
		}

		bool Loop::isProcessing(void) const
		{
			/*
				The loop node itself will only have its "isProcessing" checked once its child returns.
				Here we limit the loop body to a single frame to avoid running a small loop many times in a single frame.
				So the execution sequence should go like this:
					frame:0
						push loop
						loop processing is false
						push loop child
						pop loop child
						loop processing is true (this frame already checked)
					frame:1
						loop processing is false
						push loop child
						...
			*/

			static GameBase& game = GameBase::get();
			uint64_t thisFrame = game.getFrameNumber();

			//signal this node is processing if it already looped this frame
			bool bAlreadyTickedThisFrame = lastFrameTicked == thisFrame;

			lastFrameTicked = thisFrame;

			return bAlreadyTickedThisFrame;
		}

		void Loop::resetNode()
		{
			currentLoop = 0;
			SingleChildNode::resetNode();
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Random Selector Node
		/////////////////////////////////////////////////////////////////////////////////////
		Random::Random(
			const std::string& name,
			const std::vector<ChildChance> childChances,
			const std::vector<sp<NodeBase>>& inChildren) 
			: MultiChildNode(name, inChildren)
		{
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

			std::map<std::string, NodeBase*> nameToNodeMap;

			for (size_t childIdx = 0; childIdx < children.size(); ++childIdx)
			{
				const sp<NodeBase>& child = children[childIdx];
				const std::string& childName = child->getName();
				auto find = nameToNodeMap.find(childName);
				if (find == nameToNodeMap.end())
				{
					nameToNodeMap[childName] = child.get();
				}
				else
				{
					log("Random Behavior Node", LogLevel::LOG_ERROR, " !!! Duplicate child node name! Behavior tree must have unique named nodes within a random node !!!");
				}
			}

			std::set<std::string> assignedChanceData;
			for (const ChildChance& chance : childChances)
			{
				auto nodeFind = nameToNodeMap.find(chance.childName);
				if (nodeFind != nameToNodeMap.end())
				{
					NodeBase* childRawPtr = nodeFind->second;
					for (size_t chancePoint = 0; chancePoint < chance.chancePoints; ++chancePoint)
					{
						chanceBucket.push_back(childRawPtr);
					}
					assignedChanceData.insert(chance.childName);
				}
				else
				{
					log("Random Behavior Node", LogLevel::LOG_ERROR, "!!! Cannot find child for specified chance !!!");
				}
			}

			//make sure all children have assigned chances
			for (auto childIter : nameToNodeMap)
			{
				auto findIfAssignedChance = assignedChanceData.find(childIter.first);
				if (findIfAssignedChance == assignedChanceData.end())
				{
					std::string failedAssignment = std::string("!!! Could not assign node \"") + childIter.first + std::string("\" a chance/probability; did you forget this node? this random node:") + getName();
					log("Random Behavior Node", LogLevel::LOG_ERROR, failedAssignment.c_str());
				}
			}

		}

		bool Random::hasPendingChildren() const
		{
			return !randomChoice.has_value();
		}

		void Random::resetNode()
		{
			randomChoice.reset();
			childResult.reset();
		}

		SA::BehaviorTree::NodeBase* Random::getNextChild()
		{
			//this should only ever be called if there is no current random choice (see hasPendingChildren)
			randomChoice = rng->getInt<size_t>(0, chanceBucket.size()-1);
			return chanceBucket[*randomChoice];
		}

		void Random::notifyCurrentChildResult(bool inChildResult)
		{
			childResult = inChildResult;
		}

		bool Random::resultReady() const
		{
			//alternatively the random node could keep choosing until it finds a successful child
			//but I am leaning towards the simple design. It is basically "choose one child, run that child, return its result"
			return childResult.has_value();
		}

		bool Random::result() const
		{
			return *childResult;
		}

	}

}