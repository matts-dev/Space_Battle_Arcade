#pragma once
#include "SAGameEntity.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <assert.h>

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

		public: //tree processing
			//TODO tree-only methods private?
			virtual bool hasPendingChildren() const { return false; }		//TODO make this pure virtual
			virtual bool isProcessing() const { return false; };			//TODO make this pure virtual
			virtual bool resultReady() const { return true; };				//TODO make this pure virtual
			virtual bool result() const { return true; }					//TODO make this pure virtual
			virtual void resetNode(){}										//TODO make this pure virtual
			virtual void evaluate() {}										//TODO make this pure virtual
			virtual void notifyCurrentChildResult(bool childResult) {}		//TODO make this pure virtual
			virtual NodeBase* getNextChild() { return nullptr; }

		private:
			virtual void notifyTreeEstablished() {}; //override for memory value initialization

		protected: //subclass helpers
			Memory& getMemory() 
			{ 
				assert(assignedMemory);
				return *assignedMemory; 
			}

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
			virtual void serviceTick() {}; //= 0 //#todo make pure virtual as these should be implemented by all services
			virtual void startService() {};// = 0; create service subclasses to define this behavior; ideally should be a ticking timer. //#TODO make pure virtual
			virtual void stopService() {};// = 0; //services will need to stop timers in an overload of this; be sure to call super if necessary //#TODO make pure virtual
		protected:
			float tickSecs = 0.1f;
			bool bLoop = true;
			bool bExecuteOnStart = true;
			sp<MultiDelegate<>> timerDelegate = new_sp<MultiDelegate<>>();
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Decorator node; used for setting up conditionals for executing sub-trees. Also
		//		used to abort lower priority trees on value changes
		/////////////////////////////////////////////////////////////////////////////////////
		class Decorator : public SingleChildNode
		{
		public: //#TODO make private/protected? these public methods
			Decorator(const std::string& name, const sp<NodeBase>& child) : SingleChildNode(name, child) {}
			virtual bool isProcessing() const override;; // Decorators that may not complete immediately should overload this 
			virtual void evaluate() override;
			virtual void startBranchConditionCheck(){} // = 0; #TODO make pure virtual 
			virtual bool hasPendingChildren() const override;
			virtual bool resultReady() const override { return !isProcessing(); }; //#TODO this is mirroring a lot of stuff that is in task; investigate a common base class functionality or mixin

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
			virtual bool resultReady() const override;
			virtual bool result() const override;
			virtual void evaluate() override;

			//#TODO make pure virtual!
			virtual void beginTask() {}//= 0; //override this to start deferred tasks that will update the evaluationResult

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
		};

		////////////////////////////////////////////////////////
		// Grants ability to dynamic cast memory of primitive type
		////////////////////////////////////////////////////////
		template<typename T>
		struct PrimitiveWrapper : public GameEntity
		{
			PrimitiveWrapper(const T& value) : value(value) {}
			T value;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// A behavior tree's memory. The shared state of the tree. All nodes have access
		// to the tree's memory.
		/////////////////////////////////////////////////////////////////////////////////////
		class Memory : public GameEntity
		{
			//hashmap of keys to values
			std::unordered_map<std::string, sp<GameEntity>> memory;

			///* An RAII interface for automatically broadcasting a notify after a value has been written.*/
			//template<typename T>
			//class ScopedUpdateNotifier
			//{
			//	friend Memory; //prevent user from being able to construct or cache copies of this; only memory has that privilege
			//private:
			//	ScopedUpdateNotifier(T* value, MultiDelegate<>* updateDelegate) :
			//		value(value)
			//	{}
			//	ScopedUpdateNotifier(const ScopedUpdateNotifier& copy) = default;
			//	ScopedUpdateNotifier(ScopedUpdateNotifier&& move) = default;
			//	ScopedUpdateNotifier& operator= (const ScopedUpdateNotifier& copy) = default;
			//	ScopedUpdateNotifier& operator= (ScopedUpdateNotifier&& move) = default;
			//	
			//public:
			//	~ScopedUpdateNotifier()
			//	{
			//		if (bAccessed)
			//		{
			//			updateDelegate->broadcast();
			//		}
			//	}
			//	bool hasValue() { return value != nullptr; }
			//	operator bool() { return hasValue(); }

			//	T* get()
			//	{
			//		bAccessed = true;
			//		return value;
			//	}
			//private:
			//	//These are "dumb" pointers because they will never outlive the scope
			//	T* value;
			//	MultiDelegate<>* updateDelegate;
			//	bool bAccessed = false;
			//};

		public:
			template<typename T>
			T* getValueAs(const std::string& key) 
			{ 
				auto findResult = memory.find(key);
				if (findResult != memory.end())
				{
					sp<GameEntity>& rawResult = findResult->second;
					if constexpr (std::is_base_of<GameEntity, T>())
					{
						//profiling suggests just using dynamic cast is faster than caching rtti info in hash_set<std::type_index> and bypassing with static casts. 
						return dynamic_cast<T*>(rawResult.get());
					}
					else
					{
						if (PrimitiveWrapper<T>* wrappedObj = dynamic_cast<PrimitiveWrapper<T>*>(rawResult.get()))
						{
							return &(wrappedObj->value);
						}
					}
				}
				return nullptr;
			}

			//template<typename T>
			//ScopedUpdateNotifier<T> getWriteValueAs(const std::string& key)
			//{
			//	T* value = getValueAs(key);
			//	return ScopedUpdateNotifier<T>(value, nullptr);
			//}

			template<typename T>
			T* assignValue(const std::string& key, const sp<T>& value)
			{
				static_assert(std::is_base_of<GameEntity, T>(), "Value must be of GameEntity. For primitive/integral values, use provided PrimitiveWrapper");
				memory.insert({ key, value });
				return value.get();
			}

			bool hasValue(const std::string& key)
			{
				return memory.find(key) != memory.end();
			}

			bool removeValue(const std::string& key)
			{
				return memory.erase(key) > 0;
			}
		};

		using MemoryInitializer = std::vector<std::pair<std::string, sp<GameEntity>>>;
		enum class ExecutionState : uint32_t { STARTING, POPPED_CHILD, PUSHED_CHILD, CHILD_EXECUTING };
		struct ResumeStateData
		{
			ExecutionState state;
			bool bChildResult;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// The behavior tree composed of nodes.
		/////////////////////////////////////////////////////////////////////////////////////
		class Tree : public NodeBase
		{
		public:
			Tree(const std::string& name, const sp<NodeBase>& root, std::vector<std::pair<std::string, sp<GameEntity>>> initializedMemory = {});
			void start();
			void tick(float delta_sec);
			void stop();
		private: //start up
			void possessNodes(const sp<NodeBase>& node, uint32_t& currentPriority);

		private: //node interface
			virtual bool hasPendingChildren() const override { return true; }
			virtual bool resultReady() const override { return false; }
			virtual NodeBase* getNextChild() override{ return root.get(); }
		private:
			/** The root node; tree structure does not change once defined */
			const sp<NodeBase> root;
			/** Represents the path of current nodes being executed. Life time of nodes is controlled by the root node */
			std::vector<NodeBase*> executionStack;
			sp<Memory> memory = nullptr;

			bool bExecutingTree = false;
			uint32_t numNodes = 0;
			std::optional<ResumeStateData> resumeData;
		};

	}
}