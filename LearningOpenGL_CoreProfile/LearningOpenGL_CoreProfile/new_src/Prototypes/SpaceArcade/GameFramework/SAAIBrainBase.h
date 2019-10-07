#pragma once
#include "SAGameEntity.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <assert.h>
#include "../Tools/DataStructures/MultiDelegate.h"

namespace SA
{
	//#TODO this probably needs to exist separately in another header, so  player can do pattern of having protected member return a key
	/** Special key to allows brains (ai/player) to access methods
		Only friend classes can construct these, giving the friend
		classes unique access to methods requiring an instance as
		a parameter.*/
	class BrainKey
	{
		friend class PlayerBase;
		friend class AIBrain;
	};


	class AIBrain : public GameEntity
	{
	public:
		bool awaken();
		void sleep();
		bool isActive() { return bActive; }

	protected:
		/** return true if successful */
		virtual bool onAwaken() = 0;
		virtual void onSleep() = 0;

	protected:
		/** Give subclasses easy access to a BrainKey; copy elision should occur*/
		inline BrainKey getBrainKey() { return BrainKey{}; }

	private:
		bool bActive = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Behavior Trees
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	namespace BehaviorTree
	{
		class Tree;
		class Memory;

		/////////////////////////////////////////////////////////////////////////////////////
		// Base class for behavior tree nodes
		/////////////////////////////////////////////////////////////////////////////////////
		class NodeBase : public GameEntity
		{
			friend BehaviorTree::Tree;

		public:
			NodeBase(const std::string& name) : nodeName(name) {}

		private: //tree state machine methods. End users should not be calling these methods directly.
			friend Tree;
			virtual bool hasPendingChildren() const = 0; 
			virtual bool isProcessing() const = 0; 
			virtual bool resultReady() const = 0;
			virtual bool result() const = 0;
			virtual void evaluate() = 0;
			virtual void notifyCurrentChildResult(bool childResult) = 0;
			virtual NodeBase* getNextChild() { return nullptr; }
		protected: //tree state machine methods where subclasses are required to call parent
			virtual void resetNode() { bOnExecutionStack = false; }			

			/* Notifies users that abort happened; cancel any pending timers in your override. */
			virtual void handleNodeAborted() = 0;
		private:
			virtual void notifyTreeEstablished() {}; //override for memory value initialization

		protected: //subclass helpers
			inline Memory& getMemory() const
			{ 
				assert(assignedMemory);
				return *assignedMemory; 
			}
			inline Tree& getTree() const
			{
				assert(owningTree);
				return *owningTree;
			}
			inline uint32_t getPriority() const { return priority; }
			inline bool isOnExecutionStack() { return bOnExecutionStack; }

		protected:
			/** The first descendant children of this node */
			std::vector<sp<NodeBase>> children;

		private:
			/** a user and debugging friendly name */
			std::string nodeName;

			/** Assigned by the owning tree*/
			uint32_t priority = 0;
			Tree* owningTree = nullptr;
			Memory* assignedMemory = nullptr;
			bool bOnExecutionStack = false;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Helper base class for nodes that only have a single child.
		/////////////////////////////////////////////////////////////////////////////////////
		class SingleChildNode : public NodeBase
		{
		public:
			SingleChildNode(const std::string& name, const sp<NodeBase>& child) : NodeBase(name) { children.push_back(child); }
		private:
			virtual bool resultReady() const override			{ return bChildReturned; };
			virtual bool result() const							{ return bChildExecutionResult; }
			virtual NodeBase* getNextChild() override			{ return children.size() > 0 ? children[0].get() : nullptr; }
			virtual void notifyCurrentChildResult(bool childResult) override;

		protected:
			virtual bool hasPendingChildren() const override	{ return !bChildReturned; }
			virtual void resetNode() override
			{
				bChildReturned = false;
				NodeBase::resetNode();
			}

		protected:
			bool bChildReturned = false;
			bool bChildExecutionResult = false;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Service node; used for updating values when sub-tree is being executed.
		/////////////////////////////////////////////////////////////////////////////////////
		class Service : public SingleChildNode
		{
		public: 
			Service(const std::string& name, float tickSecs, bool bLoop, const sp<NodeBase>& child);
			virtual bool isProcessing() const override { return false; }; //one may could argue that some services need to execute before their children, so this may need to be more complex
			virtual void resetNode();
			virtual void evaluate();

		private:
			/*resetNode will cover stopping service;. so overriding abort notifications is not required*/
			virtual void handleNodeAborted() override {} 

		private: //subclass required virtuals
			virtual void serviceTick() = 0;		// the built-in update method for service subclasses. This ticks at the rate of `ticSecs` and will loop if specified at construction
			virtual void startService() = 0;	// Tree has started service start; subclasses can initiate any extra timers here.
			virtual void stopService() = 0;		// Tree has ended service; subclasses should clean up their custom timers here.
		protected:
			float tickSecs = 0.1f;
			bool bLoop = true;
			bool bExecuteOnStart = true;
			sp<MultiDelegate<>> timerDelegate = new_sp<MultiDelegate<>>();
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Decorator node; used for setting up conditionals for executing sub-trees.
		//		Also for aborting child and lower priority trees 
		/////////////////////////////////////////////////////////////////////////////////////
		class Decorator : public SingleChildNode
		{
		public:
			enum class AbortType {ChildTree, LowerPriortyTrees, ChildAndLowerPriortyTrees};
			Decorator(const std::string& name, const sp<NodeBase>& child) : SingleChildNode(name, child) {}

		private: 
			virtual bool isProcessing() const override;; // Decorators that may not complete immediately should overload this 
			virtual void evaluate() override;
			virtual void startBranchConditionCheck() = 0; //"start" because conditional check may take time. Fill conditionalResult when condition has been evaluated.
			virtual bool hasPendingChildren() const override;
			virtual bool resultReady() const override { return !isProcessing(); };

		protected:
			void abortTree(AbortType abortType);

		protected:
			virtual void resetNode() override;
		protected:
			std::optional<bool> conditionalResult;
		private:
			bool bStartedDecoratorEvaluation = false;


		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Base class for task nodes; task nodes complete jobs/tasks over time.
		/////////////////////////////////////////////////////////////////////////////////////
		class Task : public NodeBase
		{
		public:
			Task(const std::string& name) : NodeBase(name) {}
			virtual void resetNode() override;	//subclasses must override this if tasks track any state (it's not advised to track state on task; instead on tree instance's memory)
			virtual bool isProcessing() const override;
			virtual bool resultReady() const override{ return !isProcessing();}
			virtual bool result() const override;
			virtual void evaluate() override;

			/* Requires user fill out evaluationResult when task is complete. Otherwise tree will not proceed because task will appear to be incomplete.*/
			virtual void beginTask() = 0; //override this to start deferred tasks that will update the evaluationResult
		protected:
			virtual void taskCleanup() = 0; //override and clean up; be sure to call super if subclassing another task

		private: //required node methods
			virtual bool hasPendingChildren() const override { return false; }			//tasks cannot have pending children
			virtual void notifyCurrentChildResult(bool childResult) override {};		//tasks cannot have pending children

		protected:
			/** present of value in optional signals that task is completed*/
			std::optional<bool> evaluationResult;
			bool bStartedTask = false;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Helper base class for shared behavior between selector and sequence nodes.
		/////////////////////////////////////////////////////////////////////////////////////
		class MultiChildNode : public NodeBase
		{
		public:
			MultiChildNode(const std::string& name, const std::vector<sp<NodeBase>>& children);

			/** Subclasses must call super so this is triggered. */
			virtual void resetNode() override { resetMultiNode(); NodeBase::resetNode(); }
			//virtual bool hasPendingChildren() { return false; }		//Subclass should implement
			//virtual bool isProcessing() { return false; };			//Subclass should implement
			//virtual bool resultReady() { return true; };				//Subclass should implement
			//virtual bool result() { return true; }					//Subclass should implement
			//virtual void evaluate() {}								//Subclass should implement
			virtual void notifyCurrentChildResult(bool childResult);
			virtual NodeBase* getNextChild() override;
		protected: 
			inline size_t getCurrentChildIdx() const { return childIdx; } //shouldn't really be known about outside of subclasses
			/** return true if there exists a new child and successfully incremented to that child*/
			bool tryIncrementChild();

		private:
			void resetMultiNode();
		private:
			size_t childIdx = 0; //private ensures child classes cannot corrupt index and we don't have to do bounds checking
		protected:
			std::vector<bool> childResults;
			std::vector<bool> childHasReturned;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Selector node; this node chooses one path of many to execute. It does so based on
		//		the result of its children. If any child returns true, that is the path that
		//		was selected.
		/////////////////////////////////////////////////////////////////////////////////////
		class Selector : public MultiChildNode
		{
		public:
			Selector(const std::string& name, const std::vector<sp<NodeBase>>& children) : MultiChildNode(name, children) {}

			virtual bool isProcessing() const { return false; }; //selector's children may be processing, but the selector should always be immediately complete
			virtual void evaluate() { /* Selectors should not need to do any evaluation/processing */}								
			virtual bool hasPendingChildren() const;
			virtual bool resultReady() const;
			virtual NodeBase* getNextChild() override;
			virtual bool result() const;	
			virtual void handleNodeAborted() override {} // reset node will cover required cleanup
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Sequence node; this node executes all its child pathes until one of the pathes fail.
		//		Unlike the selector, if a child path returns true, the sequence node will try
		//		the next child's path.
		/////////////////////////////////////////////////////////////////////////////////////
		class Sequence : public MultiChildNode
		{
		public:
			Sequence(const std::string& name, const std::vector<sp<NodeBase>>& children) : MultiChildNode(name, children) {}
			virtual bool isProcessing() const { return false; }; //sequence's children may be processing, but the sequence should always be immediately complete
			virtual void evaluate() { /* Sequences should not need to do any evaluation/processing */ }
			virtual bool hasPendingChildren() const;
			virtual bool resultReady() const;
			virtual NodeBase* getNextChild() override;
			virtual bool result() const;
			virtual void handleNodeAborted() override {} // reset node will cover required cleanup
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Tree Memory Utils
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct MemoryEntry;
		template<typename T> struct PrimitiveWrapper;
		template<typename T> class ScopedUpdateNotifier;

		////////////////////////////////////////////////////////
		// Grants ability to dynamic cast memory of primitive type
		////////////////////////////////////////////////////////
		template<typename T>
		struct PrimitiveWrapper : public GameEntity
		{
			PrimitiveWrapper(const T& value) : value(value) {}
			T value;
		};

		////////////////////////////////////////////////////////
		// The entry used for holding memory. This allows memory
		// values to have auxiliary data such as update delegates 
		// See Memory class for more information.
		////////////////////////////////////////////////////////
		using Modified_MemoryDelegate = MultiDelegate<const std::string& /*key*/, const GameEntity* /*value*/>;
		using Replaced_MemoryDelegate = MultiDelegate<const std::string& /*key*/, const GameEntity* /*oldValue*/, const GameEntity* /*newValue*/>;
		struct MemoryEntry : public GameEntity
		{
			std::string key;
			sp<GameEntity> value;
			Modified_MemoryDelegate onValueModified;
			Replaced_MemoryDelegate onValueReplaced;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// An RAII interface for automatically broadcasting a notify after a value has been replaced.
		// This is used by tree memory to return writable values that will automatically notify decorators
		// that values have been modified. This is done when the destructor is called.
		/////////////////////////////////////////////////////////////////////////////////////
		template<typename T>
		class ScopedUpdateNotifier
		{
			friend Memory; //prevent user from being able to construct or cache copies of this; only memory has that privilege
		public:
			/* Allow users to create an out value. This means users don't need access to moves/copy ctors; those ctors will allow them to cache
				scoped values which could break things.
			*/
			ScopedUpdateNotifier() = default; 

		private:
			//Forbid users from using any of these constructors; this prevents them from caching values (ie the dumb pointers) that are expected to be released
			ScopedUpdateNotifier(T* castValue, MemoryEntry* memoryEntry)
				: castValue(castValue), memoryEntry(memoryEntry)
			{
			}
			ScopedUpdateNotifier(const ScopedUpdateNotifier& copy) = default;
			ScopedUpdateNotifier(ScopedUpdateNotifier&& move) = default;
			ScopedUpdateNotifier& operator= (const ScopedUpdateNotifier& copy) = default;
			ScopedUpdateNotifier& operator= (ScopedUpdateNotifier&& move) = default;

		public:
			~ScopedUpdateNotifier()
			{
				//important that accesssed is checked; if copy-elision isn't used this will broadcast on returning copies
				if (bAccessed && memoryEntry)
				{
					memoryEntry->onValueModified.broadcast(memoryEntry->key, memoryEntry->value.get());
				}
			}

			bool hasValue() { return castValue != nullptr; }
			operator bool() { return hasValue(); }

			T& get()
			{
				assert(hasValue());
				bAccessed = true;
				return *castValue;
			}
		private:
			//These are "raw" pointers because they should never outlive the scope of this object; if they do then this is being misused (which should be hard to do)
			T* castValue;
			MemoryEntry* memoryEntry;
			bool bAccessed = false;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// A behavior tree's memory. The shared state of the tree. All nodes have access
		// to the tree's memory.
		//
		// Design concerns:
		//		-memory that is accessed for write should broadcast updates to notes when write is done; this is for decorator aborting
		//			-memory writes use RAII to broadcast when write is complete; 
		//			-RAII structure uses raw pointers as an optimization
		//			-RAII structure is designed to not be cachable by client code as it uses raw pointers rather than smart pointers
		//		-reading values should be const correct if there will be no writing
		//		-optimize access of arbitrary types; profiling suggested that dynamic_cast is more performant than caching type_index in set
		//		-class is designed to be const correct; const operations will not modify memory entries, but may modify what is within those entries.
		//		-"replace value" means the object in memory gets replaced
		//		-"modify value" means the object in memory has its properties changed, but it is still the same object.
		//		-users of delegates "modify" and "replace" may not want to resubscribe,
		//			-hence many memory entries themselves are not replaced, only what they point to.
		/////////////////////////////////////////////////////////////////////////////////////
		class Memory : public GameEntity
		{
			//hashmap of keys to values
			std::unordered_map<std::string, sp<MemoryEntry>> memory;

		public:
			template<typename T>
			const T* getReadValueAs(const std::string& key) const
			{
				if (MemoryEntry* memEntry = _getMemoryEntry(key))
				{
					if (T* castValue = _tryCast<T>(*memEntry))
					{
						return castValue;
					}
				}
				return nullptr;
			}

			/* This does not return the scoped wrapper because that requires exposing move/copy ctor; which could accidently be 
				abused to cache this value and hence allow dangling pointers */
			template<typename T>
			bool getWriteValueAs(const std::string& key, ScopedUpdateNotifier<T>& outWriteAccess)
			{
				if (MemoryEntry* memEntry = _getMemoryEntry(key))
				{
					if (T* castValue = _tryCast<T>(*memEntry))
					{
						outWriteAccess = ScopedUpdateNotifier<T>(castValue, memEntry);
						return true;
					}
				}
				
				//default state of out variable is appropriate if we could not successfully get write value as type requested.
				return false;
			}

			/** Warning: modifying values in response to this delegate will lead to infinite recursion; if required use next tick */
			Modified_MemoryDelegate& getModifiedDelegate(const std::string& key)
			{
				MemoryEntry& memEntry =  _findOrMakeMemoryEntry(key);
				return memEntry.onValueModified;
			}

			/** Warning: replacing values in response to this delegate will lead to infinite recursion; if required use next tick */
			Replaced_MemoryDelegate& getReplacedDelegate(const std::string& key)
			{
				MemoryEntry& memEntry = _findOrMakeMemoryEntry(key);
				return memEntry.onValueReplaced;
			}

			template<typename T>
			T* replaceValue(const std::string& key, const sp<T>& newValue)
			{
				static_assert(std::is_base_of<GameEntity, T>(), "Value must be of GameEntity. For primitive/integral values, use provided PrimitiveWrapper");
				
				sp<MemoryEntry> memoryEntry = nullptr;
				sp<GameEntity> oldValue = nullptr;

				auto findResult = memory.find(key);
				if (findResult != memory.end())
				{
					memoryEntry = findResult->second;
					oldValue = memoryEntry->value;
				}
				else
				{
					memoryEntry = new_sp<MemoryEntry>();
					memoryEntry->key = key;
				}
				memoryEntry->value = newValue;
				memory.insert({ key, memoryEntry });

				//must take care that old value is not ref collected before this; storing old value as shared pointer will do the trick
				memoryEntry->onValueReplaced.broadcast(key, oldValue.get(), newValue.get());
				memoryEntry->onValueModified.broadcast(key, memoryEntry->value.get());

				return newValue.get();
			}

			bool hasValue(const std::string& key)
			{
				auto findResult = memory.find(key);
				if (findResult != memory.end())
				{
					return findResult->second->value.get() != nullptr;
				}
				return false;
			}

			/** 
				Prefer this method for removing values; it will let listeners know that value has been replaced. 
				It will also not corrupt listeners to memory entries.
			*/
			bool removeValue(const std::string& key)
			{
				//we do not want to clear subscribers to memory value modified/replaced. So insert a null.
				MemoryEntry* MemEntry = _getMemoryEntry(key);
				bool bHadMemValue = MemEntry ? (MemEntry->value != nullptr) : false;
				bool bNewValueIsNull = replaceValue(key, sp<GameEntity>{nullptr}) == nullptr; //replace value this will broadcast events
				return bHadMemValue && bNewValueIsNull;
			}

			/** 
				Avoid using this method, prefer removeValue so delegates are not lost; this will have sideeffects on decorators
				Removing entry will remove all delegate listeners; this option should NOT be checked generally.
				It mostly exists in case a task wants to create new memory values at run time then clean them up.
				The normal behavior tree workflow is to specify all the memory that will be used up front.
				Then remove values as it sees fit, but leaving delegate listeners in tack so they can react
			*/
			bool eraseEntry(const std::string& key)
			{
				//this path should be avoided in 99% of cases
				// /*onErasing.broadcast(key);*/	//I want to avoid making this a thing because then users will have to subscrib to it to be safe
				//in reality removing they memory entry should be discouraged unless under very niche scenarios (task creates and them)
				//in fact, this method should probably be removed. But then there is a way to create memory entries but no way to remove them, which seems strange.
				removeValue(key);
				return memory.erase(key) > 0;
			}

		private:
			inline MemoryEntry* _getMemoryEntry(const std::string key) const
			{
				auto findResult = memory.find(key);
				if (findResult != memory.end())
				{
					return findResult->second.get();
				}
				return nullptr;
			}

			template<typename T>
			inline T* _tryCast(MemoryEntry& memoryEntry) const
			{
				if constexpr (std::is_base_of<GameEntity, T>())
				{
					//profiling suggests just using dynamic cast is faster than caching rtti info in hash_set<std::type_index> and bypassing with static casts. 
					return dynamic_cast<T*>(memoryEntry.value.get());
				}
				else
				{
					if (PrimitiveWrapper<T>* wrappedObj = dynamic_cast<PrimitiveWrapper<T>*>(memoryEntry.value.get()))
					{
						return &(wrappedObj->value);
					}
				}
				return nullptr;
			}

			MemoryEntry& _findOrMakeMemoryEntry(const std::string& key)
			{
				auto findResult = memory.find(key);
				if (findResult != memory.end())
				{
					return *(findResult->second.get());
				}
				else
				{
					replaceValue<GameEntity>(key, sp <GameEntity>{ nullptr });
					auto findNewValue = memory.find(key);
					assert(findNewValue != memory.end());
					return *(findNewValue->second.get());
				}
			}
		};

		using MemoryInitializer = std::vector<std::pair<std::string, sp<GameEntity>>>;
		using MakeChildren = std::vector<sp<NodeBase>>;
		enum class ExecutionState : uint32_t { STARTING, POPPED_CHILD, PUSHED_CHILD, CHILD_EXECUTING };
		struct ResumeStateData
		{
			ExecutionState state;
			bool bChildResult;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// The behavior tree composed of nodes.
		/////////////////////////////////////////////////////////////////////////////////////
		static constexpr bool LOG_TREE_STATE = false;	//if true, this will hit performance hard, but gives a stream of state changes.
		extern Tree* volatile targetDebugTree;			//use a debugger to set this tree and logging will only print for this instance. hint: set by name "SA::BehaviorTree::targetDebugTree ptr_address_value"
		/////////////////////////////////////////////////////////////////////////////////////
		class Tree : public NodeBase
		{
		public:
			Tree(const std::string& name, const sp<NodeBase>& root, std::vector<std::pair<std::string, sp<GameEntity>>> initializedMemory = {});
			void start();
			void tick(float delta_sec);
			void stop();

		public: //node utils
			/* Aborts all nodes with this priority or larger magnitude values; note lower numbers mean higher priority in behavior trees */
			void abort(uint32_t priority);
			uint32_t getCurrentPriority();
			Memory& getTreeMemory();

		public: //debug utils
			void makeTreeDebugTarget() { targetDebugTree = this; }
			void makeTreeDebugTarget() const { targetDebugTree = const_cast<Tree*>(this); } //this is a debug utility, so I am okay with casting away const.

		private:
			void possessNodes(const sp<NodeBase>& node, uint32_t& currentPriority);
			void processAborts(NodeBase*& inOut_CurrentNode, ExecutionState& inOut_currentState);
			void treeLog(ExecutionState state, NodeBase* currentNode, uint32_t nodesVisited);

		private: //node interface
			virtual bool hasPendingChildren() const override { return true; }
			virtual bool resultReady() const override { return false; }
			virtual NodeBase* getNextChild() override{ return root.get(); }
			virtual void handleNodeAborted() override {}
		private: //providing defaults for require node virtuals
			virtual bool isProcessing() const override { return false; };
			virtual bool result() const override { return true; }
			virtual void evaluate() override {}										
			virtual void notifyCurrentChildResult(bool childResult){};
		private:
			/** The root node; tree structure does not change once defined */
			const sp<NodeBase> root;
			/** Represents the path of current nodes being executed. Life time of nodes is controlled by the root node */
			std::vector<NodeBase*> executionStack;
			sp<Memory> memory = nullptr;
			std::optional<uint32_t> abortPriority;

			bool bExecutingTree = false;
			uint32_t numNodes = 0;
			std::optional<ResumeStateData> resumeData;
		};

	}
}