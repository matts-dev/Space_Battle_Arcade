#include "../../../Tools/DataStructures/AdvancedPtrs.h"
#include <iostream>
#include <string>

namespace SA
{

	struct TestEntity_fwp : public GameEntity
	{
		TestEntity_fwp(const std::string& name) : name(name) {}
		std::string name;
		void printName() { std::cout << name << std::endl; }
	};

	void FastWeakPointerCompileTest()
	{
		sp<TestEntity_fwp> objA = new_sp<TestEntity_fwp>("A");
		sp<TestEntity_fwp> objB = new_sp<TestEntity_fwp>("B");
		wp<TestEntity_fwp> weakA = objA;
		wp<TestEntity_fwp> weakB = objB;

		
		fwp<TestEntity_fwp> fwp1 = objA;		//test construction from shared ptr
		fwp1 = objB;							//test assignment to shared ptr

		fwp<TestEntity_fwp> fwp2 = weakB;		//test construction from weak pointer
		fwp2 = weakA;							//test assignment to weak pointer

		fwp<TestEntity_fwp> fwp3 = nullptr;		//test construction to nullptr
		fwp3 = nullptr;							//test assignment to nullptr

		fwp<TestEntity_fwp> noArgCtor;			//test no arg ctor

		//test access functions
		fwp1->printName();
		fwp2->printName();
		if (fwp1) { fwp1->printName(); }
		if (fwp2) { fwp2->printName(); }
	}

}