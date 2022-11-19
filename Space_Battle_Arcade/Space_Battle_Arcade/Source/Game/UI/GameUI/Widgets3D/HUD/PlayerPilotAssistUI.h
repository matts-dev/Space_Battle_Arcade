#pragma once
#include "GameFramework/SAGameEntity.h"
#include "Tools/DataStructures/SATransform.h"
#include <vector>

namespace SA
{
	class WorldEntity;
	class DebugLineRender;

	struct GameUIRenderData;

	////////////////////////////////////////////////////////
	// data for instance rendering
	////////////////////////////////////////////////////////
	struct FrameData_PlayerPilotAssistUI
	{
		std::vector<glm::mat4> lineData;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A UI system that renders 3d gizmos for piolots. Uses instane rendering for efficiency
	// Intended to be globally accessible during gameplay code to push data.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class PlayerPilotAssistUI : public GameEntity
	{
		using Parent = GameEntity;
	public:
		virtual void postConstruct() override;
	private: //note: these functions are private to discourage making 3d widgets in a debugRendererSTyle
		void renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color);
	private:
		void handleGameUIRender(GameUIRenderData& rd_ui);
		void handleGameUIRenderComplete();
		void renderPlayerAttacker(const WorldEntity& playerShip, const WorldEntity& attacker, size_t attackerIdx);
		void dispatchInstancedRender();
	private:
		sp<DebugLineRender> lineRenderer; //NOTE currently there is no reason for this to be different than debug renderer, so using that class. 
		FrameData_PlayerPilotAssistUI frameData;
	};
}