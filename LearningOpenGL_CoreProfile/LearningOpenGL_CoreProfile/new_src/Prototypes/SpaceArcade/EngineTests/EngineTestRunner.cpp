#include "EngineTestSuite.h"
#include <iostream>

namespace SA
{
	int RunEngineTests()
	{
		sp<SA::EngineTestSuite> engineTests = new_sp<SA::EngineTestSuite>();
		engineTests->run(false);

		std::cin.get();
		return 0;
	}
}

//int main()
//{
//	return SA::RunEngineTests();
//}