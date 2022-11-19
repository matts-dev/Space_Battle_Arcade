#include "LifetimePointerSyntaxTest.h"

#include <iostream>
#include <string>
#include "Tools/DataStructures/AdvancedPtrs.h"
#include "Tools/DataStructures/LifetimePointer.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SAGameEntity.h"

namespace
{
	struct TestBase : public SA::GameEntity
	{
		TestBase(const std::string& name) : name(name) {}
		std::string name;
		void doWork() { std::cout << ""; }
	};

	struct TestChild : public TestBase
	{
		TestChild() : TestBase("child test") {}
		void requestDestroy() { destroy(); }
	};

	struct DeleteOnDtor : public SA::GameEntity
	{
		DeleteOnDtor(SA::LifetimePtrTest& test) : test(test)
		{}
		~DeleteOnDtor()
		{
			if (test.hasFinishedFirstTick())
			{
				test.markDeferredDeleteComplete();
			}
			else
			{
				test.completeTest("deferred delete test", "deferred delete happened too early");
			}
		}
		SA::LifetimePtrTest& test;
	};
}

namespace SA
{
	void LifetimePtrTest::beginTest()
	{
		LiveTest::beginTest();

		//many of these tests just test code syntax and make sure it compiles.
		sp<TestBase> objA = new_sp<TestBase>("A");
		sp<TestBase> objB = new_sp<TestBase>("B");

		wp<TestBase> weakA = objA;
		wp<TestBase> weakB = objB;

		lp<TestBase> lp1 = objA;		//test construction from shared ptr
		lp1 = objB;								//test assignment to shared ptr

		lp<TestBase> lp2 = weakB;		//test construction from weak pointer
		lp2 = weakA;							//test assignment to weak pointer

		lp<TestBase> lp3 = nullptr;	//test construction to nullptr
		lp3 = nullptr;							//test assignment to nullptr

		lp<TestBase> noArgCtor;		//test no arg ctor

		//test access functions
		lp1->doWork();
		lp2->doWork();
		if (lp1) { lp1->doWork(); }
		if (lp2) { lp2->doWork(); }

		//test implicit conversions
		sp<TestChild> childA = new_sp<TestChild>();
		wp<TestChild> weakChild = childA;
		lp<TestBase> lpBase1 = childA; //from sp
		lp<TestBase> lpBase2 = weakChild;//from wp
		passingTests.insert("basic smart pointer implicit conversion syntax passed.");

		//check special member functions within lp
		lp<TestChild> lpToMove = new_sp<TestChild>(); //not advised to have an lp owning a pointer; lp should be used as handles
		lp<TestChild> lpToChild1 = childA;				//copy ctor
		lp<TestChild> lpToChild2;						//no arg
		lpToChild2 = childA;							//copy assignment
		lp<TestChild> lpToChild3 = lpToChild1;			//copy ctor
		lp<TestChild> lpToChild4 = std::move(lpToMove);	//move ctor
		lpToChild4 = std::move(lpToChild4);				//move assignment to self
		assert(lpToChild4);								//make sure cannot move self
		lpToChild4 = lpToChild4;						//copy assignment to self
		assert(lpToChild4);
		lp<TestChild> lpToMove2 = childA;				//copy ctor
		lpToChild4 = std::move(lpToMove2);				//move assignment
		passingTests.insert("constructor syntax test.");

		sp<TestChild> cmpChildSP1 = new_sp<TestChild>();
		sp<TestChild> cmpChildSP2 = new_sp<TestChild>();
		sp<TestBase> cmpBaseSP = new_sp<TestBase>("cmp");
		lp<TestChild> cmpChildLP1 = cmpChildSP1;
		lp<TestChild> cmpChildLP2 = cmpChildSP2;
		lp<TestBase> cmpBaseLP1 = cmpChildLP1;
		wp<TestBase> cmpBaseWP1 = cmpChildSP1;

		completeTest((cmpChildLP1 == cmpChildLP1), "comparison operator", "same object does not pass == test on itself.");
		completeTest(!(cmpChildLP1 == cmpChildLP2), "comparison operator", "different objects identified as same object.");
		completeTest((cmpChildLP1 != cmpChildLP2), "comparison operator", "!= operator did not work correctly on same typed pointers.");
		completeTest((cmpChildLP1 == cmpBaseLP1), "comparison operator", "different typed pointers to same object did not identify as being equal.");
		completeTest(!(cmpChildLP2 == cmpBaseLP1), "comparison operator", "different typed pointers to different object appeared equal based on ==.");
		completeTest((cmpChildSP1 == cmpChildLP1) && (cmpChildLP1 == cmpChildSP1) && (cmpBaseLP1 == cmpChildSP1) && (cmpChildSP1 == cmpBaseLP1)
			&& (cmpChildSP2 != cmpChildLP1) && !(cmpChildSP2 == cmpChildLP1),
			"comparison operator", "comparision to standard shared pointers did not work as expected.");
		//make sure we can't compare weak pointer with ==, then comment it out after getting compile error
		//bool bWeakComparison = (cmpBaseWP1 == cmpChildLP1); //uncomment this code to test compiler error message.

		//check casting
		lp<TestChild> lpChildPtr = new_sp<TestChild>();
		lp<TestBase> lpBasePtr_constructed = lpChildPtr;
		lp<TestBase> lpBasePtr_assigned;
		lpBasePtr_assigned = lpChildPtr;
		completeTest(true, "implicit upcasting syntax", "");

		sp<TestChild> toSPChild = lpChildPtr;
		sp<TestBase> toSPBase = static_cast<sp<TestChild>>(lpChildPtr); //two implicit conversion are not allowed; perhaps this can be worked around
		wp<TestChild> toWPChild = lpChildPtr;
		wp<TestBase> toWPBase = static_cast<sp<TestChild>>(lpChildPtr);
		completeTest(toSPBase && toSPChild && !toWPBase.expired() && !toWPChild.expired(), "conversion to smart pointers", "conversion did not work successfully");

		bool sampePtr = (lpChildPtr == lpBasePtr_constructed);
		bool shouldFailDiffPtr = lpBasePtr_constructed == lpToChild1;
		assert(sampePtr);
		assert(!shouldFailDiffPtr);

		//test destroyed event nulling out the lifetime pointer
		sp<TestChild> destroyChild_typed_sp = new_sp<TestChild>();
		destroy_sp = destroyChild_typed_sp;
		destroy_wp = destroyChild_typed_sp;
		destroy_lp = destroy_sp;
		bIsNonNullAtStart_destroyTest = bool(destroy_lp);
		destroyChild_typed_sp->requestDestroy();
		//destroy requires a frame for processing, so check later

		//test downcasting through shared_ptr conversion
		sp<TestChild> downcast_sp = new_sp<TestChild>();
		lp<TestBase> downcast_base_lp = downcast_sp;
		lp<TestChild> downcast_child_lp = std::dynamic_pointer_cast<TestChild>(downcast_base_lp.toSP());
		completeTest(downcast_child_lp, "down cast test", "down cast was null when it should have been a valid pointer");

		//test that lifetime owned pointer gets cleaned up
		lp<TestChild> cleanup_lp = new_sp<TestChild>();
		cleanup_wp = cleanup_lp.toSP();
		cleanup_lp->requestDestroy();

		//test that lp reject pointers that are pending destroy
		sp<TestChild> toDelete = new_sp<TestChild>();
		toDelete->requestDestroy();
		lp<TestChild> shouldBeNull = toDelete;
		completeTest(!bool(shouldBeNull), "Test lp construct to null on pending delete", "lp did not become null when provided a pending delete pointer");

		//test deferred delete
		{
			lp<DeleteOnDtor> deferredDelete = new_sp<DeleteOnDtor>(*this); //when this leaves scope it will try to delete, but the deferred delete system will cause it ot delete on next tick.
		}


		//test container of lifetime pointer updates -- observing container isnt clearing pointer
		{
			sp<TestChild> childInStack1 = new_sp<TestChild>();
			sp<TestChild> childInStack2 = new_sp<TestChild>();

			lifetimePointerStack.push(childInStack1);
			lifetimePointerStack.push(childInStack2);

			childInStack1->requestDestroy();
			childInStack2->requestDestroy();
		}
	}

	bool LifetimePtrTest::passedTest()
	{
		return failingTests.size() == 0;
	}

	void LifetimePtrTest::completeTest(bool bPassed, const std::string& subtestName, const std::string& failReason)
	{
		if (bPassed)
		{
			passingTests.insert(subtestName);
		}
		else
		{
			//just let test results overwrite each other -- not ideal but very niche case that test will fail in two different ways. 
			//If so, fiing on issue will then show another issue to be fixed. Ideally programmer should see both reasons at once
			//to not end up in a circular fixing cycle, but that is very unlikely considering the scope of this test.
			//assert(failingTests.find(subtestName) == failingTests.end()); 

			failingTests.insert({ subtestName, failReason });
		}
	}

	void LifetimePtrTest::tick()
	{
		tickNum++;
		//wait until the third tick to make sure the lifetime owned pointer was deleted by the deferred deleteer.
		//tick0 : delete pointer
		//tick1 : system ticker will clean up pointer (may come after test tick)
		//tick2 : test tick safe to check if pointer was deleted
		const int ticksToWait = 2;
		if (tickNum == ticksToWait)
		{
			//test that lp owned resources are deleted later, rather than frame that lp goes out of scope.
			completeTest(bDeferredDeleteComplete, "deferred delete test", "failed to delete pointer on next tick");

			//test that the destroy event correctly clears out lp
			{
				bool bNullAfterDestroy = !bool(destroy_lp);
				completeTest(bNullAfterDestroy && bIsNonNullAtStart_destroyTest, "destroy test", "destroying the object did not null out the lifetime pointer");
				completeTest(bool(destroy_sp), "destroy test", "the destroyed sp was not valid; something is likely wrong with the test");
				destroy_sp = nullptr;
				completeTest(destroy_wp.expired(), "destroy test", "the weak pointer to destroyed object was still valid after all references should have been cleaned up");
				completeTest(cleanup_wp.expired(), "lp owned cleanup test", "the weak pointer pointing to the resource to be cleaned up by the lp was non null after cleanup");
			}

			//test container updating 
			{
				lp<GameEntity> stackPtr1 = lifetimePointerStack.top();
				lifetimePointerStack.pop();
				lp<GameEntity> stackPtr2 = lifetimePointerStack.top();
				completeTest((!stackPtr1 && !stackPtr2), "test container of lp destroy updates", "pointers wre still valid after being destroyed");
			}

			char msg[2048];
			constexpr size_t msgSize = sizeof(msg) / sizeof(msg[0]);

			for (const std::string passingTestName : passingTests)
			{
				std::string passed = "PASSED";
				snprintf(msg, msgSize, "\t %s : %s", passed.c_str(), passingTestName.c_str());
				log("LifetimePtr Tests", LogLevel::LOG, msg);
			}

			for (auto& kvPair : failingTests)
			{
				std::string result = "\tFAILED";

				snprintf(msg, msgSize, "\t %s : %s - %s", result.c_str(), kvPair.first.c_str(), kvPair.second.c_str());
				log("LifetimePtr Tests", LogLevel::LOG, msg);
			}

			snprintf(msg, msgSize, "%s : Ending LifetimePtr Tests", passedTest() ? "PASSED" : "FAILED");
			log("LifetimePtr Tests", LogLevel::LOG, msg);

			bComplete = true;
		}
		assert(tickNum <= ticksToWait);
		
	}
}
