#pragma once
#include "GameFramework/SAGameEntity.h"
#include <vector>
#include <string>
#include <iostream>

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Unit Test Base Class
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class UnitTest : public GameEntity
	{
	public:
		virtual bool run(bool stopOnFail = false)
		{
			bool passed = runInternal(stopOnFail);
			if (passed)
			{
				std::cout << "\tpassed: "<< testNamespace << " " << testName << std::endl;
			}
			else
			{
				std::cerr << "FAILED: " << testNamespace << " " << testName << " | " << errorMessage << std::endl;
			}
			return passed;
		}

	protected:
		virtual bool runInternal(bool bStopOnFail) = 0;

	protected:
		std::string testName = "no_name_given";
		std::string testNamespace = "";
		std::string errorMessage = "";
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Test Suite base class.
	//
	// A test suit is a unit test to allow for nesting of test cases.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TestSuite : public UnitTest
	{
	public:
		virtual bool run(bool stopOnFail) override
		{
			//don't print fail test for test suites, override if you want this behavior in nesting subclasses
			return runInternal(stopOnFail);
		}

	protected:
		virtual bool runInternal(bool stopOnFail) override
		{
			bool passed = true;
			for (sp<UnitTest>& test : tests)
			{
				passed &= test->run(stopOnFail);
				if (!passed && stopOnFail) break;
			}
			return passed;
		}

	protected:
		void addTest(const sp<UnitTest>& newTest)
		{
			tests.push_back(newTest);
		}

	private:
		std::vector< sp<UnitTest> > tests;
	};

	class EngineTestSuite : public TestSuite
	{
	public:
		EngineTestSuite();
	};
}
