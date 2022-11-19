#include "Tools/DataStructures/AdvancedPtrs.h"
#include <iostream>
#include <string>

namespace
{
	struct TestEntityBase_fwp : public SA::GameEntity
	{
		TestEntityBase_fwp(const std::string& name) : name(name) {}
		std::string name;
		void printName() { std::cout << name << std::endl; }
	};

	struct TestFWPChild : public TestEntityBase_fwp
	{
		TestFWPChild() : TestEntityBase_fwp("child test") {}
	};
}

namespace SA
{
	void FastWeakPointerCompileTest()
	{
		sp<TestEntityBase_fwp> objA = new_sp<TestEntityBase_fwp>("A");
		sp<TestEntityBase_fwp> objB = new_sp<TestEntityBase_fwp>("B");
		wp<TestEntityBase_fwp> weakA = objA;
		wp<TestEntityBase_fwp> weakB = objB;

		
		fwp<TestEntityBase_fwp> fwp1 = objA;		//test construction from shared ptr
		fwp1 = objB;							//test assignment to shared ptr

		fwp<TestEntityBase_fwp> fwp2 = weakB;		//test construction from weak pointer
		fwp2 = weakA;							//test assignment to weak pointer

		fwp<TestEntityBase_fwp> fwp3 = nullptr;		//test construction to nullptr
		fwp3 = nullptr;							//test assignment to nullptr

		fwp<TestEntityBase_fwp> noArgCtor;			//test no arg ctor

		//test access functions
		fwp1->printName();
		fwp2->printName();
		if (fwp1) { fwp1->printName(); }
		if (fwp2) { fwp2->printName(); }

		//test implicit conversions
		sp<TestFWPChild> childA = new_sp<TestFWPChild>();
		wp<TestFWPChild> weakChild = childA;
		fwp<TestEntityBase_fwp> weakBase1 = childA;
		fwp<TestEntityBase_fwp> weakBase2 = weakChild;
	}

}