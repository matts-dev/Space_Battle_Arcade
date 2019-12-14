#include "../../../Tools/DataStructures/AdvancedPtrs.h"
#include <iostream>
#include <string>

namespace
{
	struct TestEntityBase_lp : public SA::GameEntity
	{
		TestEntityBase_lp(const std::string& name) : name(name) {}
		std::string name;
		void printName() { std::cout << name << std::endl; }
	};

	struct TestFWPChild : public TestEntityBase_lp
	{
		TestFWPChild() : TestEntityBase_lp("child test") {}
	};
}

namespace SA
{

	void LifetimePointerCompileTest()
	{
		//sp<TestEntityBase_lp> objA = new_sp<TestEntityBase_lp>("A");
		//sp<TestEntityBase_lp> objB = new_sp<TestEntityBase_lp>("B");

		//wp<TestEntityBase_lp> weakA = objA;
		//wp<TestEntityBase_lp> weakB = objB;

		//lp<TestEntityBase_lp> lp1 = objA;		//test construction from shared ptr
		//lp1 = objB;								//test assignment to shared ptr

		//lp<TestEntityBase_lp> lp2 = weakB;		//test construction from weak pointer
		//lp2 = weakA;							//test assignment to weak pointer

		//lp<TestEntityBase_lp> lp3 = nullptr;	//test construction to nullptr
		//lp3 = nullptr;							//test assignment to nullptr

		//lp<TestEntityBase_lp> noArgCtor;			//test no arg ctor

		////test access functions
		//lp1->printName();
		//lp2->printName();
		//if (lp1) { lp1->printName(); }
		//if (lp2) { lp2->printName(); }

		////test implicit conversions
		//sp<TestFWPChild> childA = new_sp<TestFWPChild>();
		//wp<TestFWPChild> weakChild = childA;
		//lp<TestEntityBase_lp> weakBase1 = childA;
		//lp<TestEntityBase_lp> weakBase2 = weakChild;

		//#TODO test const updates
		

	}

}