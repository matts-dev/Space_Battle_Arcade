#include "EngineTestSuite.h"

//forward declarations to get the test suites (that way we don't need headers for each of these)
sp<SA::TestSuite> getDelegateTestSuite();

namespace SA
{
	EngineTestSuite::EngineTestSuite()
	{
		addTest(getDelegateTestSuite());
	}
}

