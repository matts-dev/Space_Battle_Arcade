#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>
#include "SATComponent.h"
#include <assert.h>
#include <iostream>
#include <memory>

namespace SAT
{
	class KeyFrameAgent;
	class KeyFrame;
	class TickUnitTest;

	struct ColumnBasedTransform
	{
		glm::vec3 position = { 0, 0, 0 };
		glm::vec3 rotationDeg = { 0, 0, 0 };
		glm::vec3 scale = { 1, 1, 1 };

		glm::mat4 getModelMatrix()
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = glm::rotate(model, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, scale);
			return model;
		}
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  Unit Test Base Classes:
	//
	//	A TestSuite is a collection of unit testes to run.
	//
	//  A Unit Test is a simple test that manipulates shapes in some way over time (through a tick method).
	//
	//	A Unit Test consists of a sequence of key frames, in a similar spirit to how animations are divided into key frames.
	//	Key frames allow for complex unit test behavior. Such as: "walk down for 1 seconds, the walk right for 2 secodns"
	//
	//	Each KeyFrame has KeyFrameAgents.  Each agent represents a shape that may be intractable. So, a KeyFrame may have multiple agents
	//	moving independently.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class KeyFrameAgent
	{
	public: //funcs
		KeyFrameAgent(SATShape& shape) : shape(shape) {}
		const SATShape& getShape() { return shape; }
		const bool failedTick() { return bTickFailed; }

		//called when keyframe is complete, returns true if in passing position for keyframe test
		virtual bool correctAtKeyFrameComplete() { return false; }
		virtual void tick(float deltaTimeSecs) {}
		virtual void initializeKeyFrameSetup() {}

	protected: //vars
		SATShape& shape;

	protected: //funcs
		std::vector<std::shared_ptr<KeyFrameAgent>>& GetAllKeyFrameAgents();
		bool bTickFailed = false;

	private: //vars
		friend KeyFrame;
		std::shared_ptr<KeyFrame> owningKeyFrame;
	};

	class KeyFrame : public std::enable_shared_from_this<KeyFrame>
	{
	public: 
		KeyFrame(float inFrameDurationSecs) : frameDurationSecs(inFrameDurationSecs) {}

		void AddKeyFrameAgent(std::shared_ptr<KeyFrameAgent> newAgent);

	private:
		friend TickUnitTest;
		friend KeyFrameAgent;

		std::vector<std::shared_ptr<KeyFrameAgent>> frameAgents;
		float frameDurationSecs;
	};

	class TickUnitTest
	{
	public:
		void setFrame(uint32_t newFrameIdx);
		bool isComplete();
		void tick(float deltaTimeSecs);
		void AddKeyFrame(std::shared_ptr<KeyFrame> newFrame) { frames.push_back(newFrame); }

	public:
		std::vector<std::shared_ptr<KeyFrame>> frames;
		uint32_t frameIdx = 0;
		float currentFrameDurationSecs = 0;
		bool failed = false;
	};

	class TestSuite
	{
	public:
		std::shared_ptr<SAT::TickUnitTest> getCurrentTest();
		void restartCurrentTest();
		void restartAllTests();
		void start();
		void stop();
		void safeCorrectIndex();
		void nextTest();
		void previousTest();
		void AddTest(std::shared_ptr<TickUnitTest> test);
		void tick(float deltaTime);
		bool isRunning();

	private:
		uint32_t currentTestIdx;
		std::vector<std::shared_ptr<TickUnitTest>> UnitTests;
		bool runTests = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Simple Velocity Applied Tests
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ApplyVelocityKeyFrame : public KeyFrame
	{
	public:
		ApplyVelocityKeyFrame(float KeyFrameDurationSecs) : KeyFrame(KeyFrameDurationSecs) {}
	};

	struct ApplyVelocityFrameAgent : public KeyFrameAgent
	{
		ApplyVelocityFrameAgent(
			SATShape& shape,
			glm::vec3 velocity,
			ColumnBasedTransform& localTransform,
			ColumnBasedTransform startTransform,
			bool(*InCustomCOmpleteFunc)(ApplyVelocityFrameAgent&)
		) 
		: KeyFrameAgent(shape), 
			velocity(velocity),
			localTransform(localTransform),
			startTransform(startTransform),
			customCompleteFunc(InCustomCOmpleteFunc)
		{
		}
	public:
		virtual bool correctAtKeyFrameComplete() override;
		virtual void tick(float deltaTimeSecs) override;
		virtual void initializeKeyFrameSetup() override;

		void SetCustomCompleteTestFunc(bool(*inCustomCompleteFunc)(ApplyVelocityFrameAgent&)) {	customCompleteFunc = inCustomCompleteFunc; }
		void SetCustomTickFailFunc(bool(*inCustomTickFailCheck)(ApplyVelocityFrameAgent&)) { CustomTickFailCheck = inCustomTickFailCheck; }
		void SetStartTransform(const ColumnBasedTransform& newStartTransform) { startTransform = newStartTransform; }
		void SetNewVelocity(glm::vec3 newVelocity) { velocity = newVelocity; }

		const ColumnBasedTransform& GetTransform() { return localTransform; }
		const ColumnBasedTransform& GetStartTransform() { return startTransform; }

		void markFailedTickTest() { bTickFailed = true; }

	private:
		glm::vec3 velocity;
		ColumnBasedTransform& localTransform;
		ColumnBasedTransform startTransform;
		bool(*customCompleteFunc)(ApplyVelocityFrameAgent& thisInstance) = [](ApplyVelocityFrameAgent&) -> bool {return false; };
		bool(*CustomTickFailCheck)(ApplyVelocityFrameAgent& thisInstance) = [](ApplyVelocityFrameAgent&) -> bool {return false; };

	};

	class ApplyVelocityTest : public TickUnitTest
	{
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





}