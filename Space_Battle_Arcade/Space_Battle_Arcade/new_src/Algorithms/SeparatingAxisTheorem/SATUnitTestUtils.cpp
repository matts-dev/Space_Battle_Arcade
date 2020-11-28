#include "SATUnitTestUtils.h"

template<typename T>
using sp = std::shared_ptr<T>;


void SAT::KeyFrame::AddKeyFrameAgent(std::shared_ptr<KeyFrameAgent> newAgent)
{

	frameAgents.push_back(newAgent);
	newAgent->owningKeyFrame = shared_from_this();
}


std::shared_ptr<SAT::TickUnitTest> SAT::TestSuite::getCurrentTest()
{
	if (!UnitTests.size())
	{
		throw std::runtime_error("NO UNIT TESTS");
	}

	if (currentTestIdx < 0)
	{
		return UnitTests[0];
	}
	else if (currentTestIdx > UnitTests.size() - 1)
	{
		return UnitTests[UnitTests.size() - 1];
	}
	else
	{
		return UnitTests[currentTestIdx];
	}
}

void SAT::TestSuite::restartCurrentTest()
{
	sp<TickUnitTest> Test = getCurrentTest();
	Test->setFrame(0);
	start();
}

void SAT::TestSuite::restartAllTests()
{
	currentTestIdx = 0;
	start();
}

void SAT::TestSuite::start()
{
	getCurrentTest()->setFrame(0);
	runTests = true;
	

}

void SAT::TestSuite::stop()
{
	runTests = false;
}

void SAT::TestSuite::safeCorrectIndex()
{
	currentTestIdx = currentTestIdx < 0 ? 0 : currentTestIdx;
	currentTestIdx = currentTestIdx >= UnitTests.size() ? UnitTests.size() -1 : currentTestIdx;
}

void SAT::TestSuite::nextTest()
{
	currentTestIdx++;
	safeCorrectIndex();
	start();
}

void SAT::TestSuite::previousTest()
{
	currentTestIdx--;
	safeCorrectIndex();
	start();
}

void SAT::TestSuite::AddTest(std::shared_ptr<TickUnitTest> test)
{
	UnitTests.push_back(test);
}

void SAT::TestSuite::tick(float deltaTime)
{
	if (!runTests)
	{
		return;
	}

	sp<TickUnitTest> currentTest = getCurrentTest();
	if (currentTest->isComplete())
	{
		if (!currentTest->failed)
		{
			std::cout << "PASSED TEST" << std::endl;
			if (currentTestIdx != UnitTests.size() - 1)
			{
				nextTest();
				currentTest = getCurrentTest();
			}
			else
			{
				std::cout << "tests complete" << std::endl;
				runTests = false;
			}
		}
		else
		{
			std::cerr << "FAILED TEST" << std::endl;
			runTests = false;
		}
	}

	if(runTests)
	{
		currentTest->tick(deltaTime);
	}
}

bool SAT::TestSuite::isRunning()
{
	return runTests;
}

void SAT::TickUnitTest::setFrame(uint32_t newFrameIdx)
{
	frameIdx = newFrameIdx;
	if (newFrameIdx < frames.size() && newFrameIdx >= 0)
	{
		currentFrameDurationSecs = 0;
		failed = false;

		sp<KeyFrame>& frame = frames[newFrameIdx];
		for (sp<KeyFrameAgent>& agent : frame->frameAgents)
		{
			agent->initializeKeyFrameSetup();
		}
	}
}

bool SAT::TickUnitTest::isComplete()
{
	return frameIdx >= frames.size() || frameIdx < 0 || failed;
}

void SAT::TickUnitTest::tick(float deltaTimeSecs)
{
	if (frameIdx < frames.size() && !failed)
	{
		sp<KeyFrame>& frame = frames[frameIdx];
		currentFrameDurationSecs += deltaTimeSecs;

		//TICK FRAME
		if (currentFrameDurationSecs < frame->frameDurationSecs)
		{
			for (sp<KeyFrameAgent>& agent : frame->frameAgents)
			{
				agent->tick(deltaTimeSecs);
			}
		}
		else //END FRAME
		{
			for (sp<KeyFrameAgent>& agent : frame->frameAgents)
			{
				if (!agent->correctAtKeyFrameComplete() || agent->failedTick())
				{
					failed = true;
					std::cerr << "\tFAILED KEY FRAME" << std::endl;
					return; //freeze objects in failing state
				}
			}
			std::cerr << "\tPASSED KEY FRAME" << std::endl;
			setFrame(frameIdx + 1);
		}
	}
}

bool SAT::ApplyVelocityFrameAgent::correctAtKeyFrameComplete()
{
	return customCompleteFunc(*this);
}

void SAT::ApplyVelocityFrameAgent::tick(float deltaTimeSecs)
{
	//don't attempt move if zero velocity; don't let MTV move this object if another object is failing to detect collision
	if (velocity == glm::vec3(0.0f))
	{
		return;
	}

	//do move then check collision
	localTransform.position += velocity * deltaTimeSecs;
	shape.updateTransform(localTransform.getModelMatrix());

	for (sp<KeyFrameAgent>& agentBase : GetAllKeyFrameAgents())
	{
		if (agentBase.get() != this)
		{
			//don't worry about overhead of std::dynamic_pointer_cast; we're not going to hold a new shared pointer
			if (SAT::ApplyVelocityFrameAgent* agent = dynamic_cast<SAT::ApplyVelocityFrameAgent*>( agentBase.get() ) )
			{
				glm::vec4 mtv;

				//only good for testing 2 object collisions as MTV may slide object into another
				if (Shape::CollisionTest(shape, agent->shape, mtv))
				{
					localTransform.position += glm::vec3(mtv);
					shape.updateTransform(localTransform.getModelMatrix());
				}
			}
		}

		CustomTickFailCheck(*this);
	}
}

void SAT::ApplyVelocityFrameAgent::initializeKeyFrameSetup()
{
	localTransform = startTransform;
}

std::vector<std::shared_ptr<SAT::KeyFrameAgent>>& SAT::KeyFrameAgent::GetAllKeyFrameAgents()
{
	return owningKeyFrame->frameAgents;
}
