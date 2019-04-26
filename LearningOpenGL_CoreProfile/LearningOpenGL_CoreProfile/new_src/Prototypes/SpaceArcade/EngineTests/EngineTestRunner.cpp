#include "..\Tools\SmartPointerAlias.h"
#include "EngineTestSuite.h"
#include <iostream>

namespace
{
	int trueMain()
	{
		sp<SA::EngineTestSuite> engineTests = new_sp<SA::EngineTestSuite>();
		engineTests->run(false);

		std::cin.get();
		return 0;
	}
}

//int main()
//{
//	return trueMain();
//}