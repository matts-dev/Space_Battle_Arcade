#include "EngineTestSuite.h"

//forward declarations to get the test suites (that way we don't need headers for each of these)

namespace SA
{
	sp<SA::TestSuite> getDelegateTestSuite();

	EngineTestSuite::EngineTestSuite()
	{
		addTest(getDelegateTestSuite());
	}
}

