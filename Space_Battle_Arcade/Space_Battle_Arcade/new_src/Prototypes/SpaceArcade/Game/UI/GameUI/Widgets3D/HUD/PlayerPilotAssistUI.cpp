#include "PlayerPilotAssistUI.h"
#include "../../../../SpaceArcade.h"
#include "../../../../GameSystems/SAUISystem_Game.h"
#include "../../../../../GameFramework/SAPlayerSystem.h"
#include "../../../../../GameFramework/SAGameBase.h"
#include "../../../../../GameFramework/SAPlayerBase.h"
#include "../../../../../GameFramework/Components/GameplayComponents.h"
#include "../../../../../GameFramework/Interfaces/SAIControllable.h"
#include "../../../../../GameFramework/SABehaviorTree.h"
#include "../../../../AI/SAShipBehaviorTreeNodes.h"
#include "../../../../../GameFramework/SADebugRenderSystem.h"
#include "../../../../AI/GlobalSpaceArcadeBehaviorTreeKeys.h"
#include "../../../../../Tools/color_utils.h"
#include "../../../../../Rendering/OpenGLHelpers.h"
#include "../../../../../Rendering/Camera/SACameraBase.h"

namespace SA
{
	using namespace glm;

	void PlayerPilotAssistUI::postConstruct()
	{
		Parent::postConstruct();

		lineRenderer = new_sp<DebugLineRender>();

		/////////////////////////////////////////////////////////////////////////////////////
		// bind events
		/////////////////////////////////////////////////////////////////////////////////////
		SpaceArcade::get().getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &PlayerPilotAssistUI::handleGameUIRender);
		SpaceArcade::get().getGameUISystem()->onUIGameRenderComplete.addWeakObj(sp_this(), &PlayerPilotAssistUI::handleGameUIRenderComplete);
	}

	void PlayerPilotAssistUI::handleGameUIRender(GameUIRenderData& rd_ui)
	{
		const std::vector<sp<PlayerBase>>& players = SpaceArcade::get().getPlayerSystem().getAllPlayers();
		for (const sp<PlayerBase>& player : players)
		{
			if (IControllable* controlTarget = player->getControlTarget())
			{
				if (WorldEntity* controlTarget_we = controlTarget->asWorldEntity())
				{
					if (const BrainComponent* ctBrainComp = controlTarget_we->getGameComponent<BrainComponent>())
					{
						if (const BehaviorTree::Tree* tree = ctBrainComp->getTree())
						{
							using namespace BehaviorTree;
							Memory& memory = tree->getMemory();

							////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							// render player attackers
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							if (const BehaviorTree::ActiveAttackers* attackers = memory.getReadValueAs<BehaviorTree::ActiveAttackers>(BT_AttackersKey))
							{
								size_t attackerIdx = 0;
								for (auto& iter : *attackers)
								{
									if (iter.second.attacker)
									{
										renderPlayerAttacker(*controlTarget_we, *iter.second.attacker.fastGet(), attackerIdx);
									}
									attackerIdx++;
								}
							}
						}
					}
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// dispatch the accumulated instance render
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		dispatchInstancedRender();
	}

	void PlayerPilotAssistUI::renderPlayerAttacker(const WorldEntity& playerShip, const WorldEntity& attacker, size_t attackerIdx)
	{
		//TEMP
		vec3 playerPos = playerShip.getWorldPosition();
		vec3 attackerPos = attacker.getWorldPosition();

		//tweak the color based on this frames number of attackers so that user can easily track who they are trying to destroy
		vec3 scaledRed = color::red() * clamp(1.f - (attackerIdx / 4.f), 0.25f, 1.f);

		if constexpr (constexpr bool bSimpleVersion = false)
		{
			renderLine(playerPos, attackerPos, scaledRed);
		}
		else
		{
			vec3 toAttacker_n = attackerPos - playerPos;
			float distToAttacker = glm::length(toAttacker_n);
			toAttacker_n /= distToAttacker;

			constexpr float offsetDist = 1.f;

			if (distToAttacker > 2 * offsetDist)
			{
				vec3 offset_v = toAttacker_n * offsetDist;
				renderLine(playerPos + offset_v, attackerPos - offset_v, scaledRed);
			}
		}
	}

	void PlayerPilotAssistUI::renderLine(const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color)
	{
		mat4 shearMatrix = mat4(vec4(pntA, 0), vec4(pntB, 0), vec4(color, 1), vec4(0.f, 0.f, 0.f, 1.f));
		frameData.lineData.push_back(shearMatrix);
	}

	void PlayerPilotAssistUI::dispatchInstancedRender()
	{
		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr);

		if (frameData.lineData.size() > 0)
		{

			GLuint lineVAO = lineRenderer->getVAOs()[0];
			ec(glBindVertexArray(lineVAO));

			ec(glBindBuffer(GL_ARRAY_BUFFER, lineRenderer->getMat4InstanceVBO()));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * frameData.lineData.size(), &frameData.lineData[0], GL_STATIC_DRAW));

			GLsizei numVec4AttribsInBuffer = 4;
			size_t packagedVec4Idx_matbuffer = 0;

			//load shear packed matrix
			ec(glEnableVertexAttribArray(1));
			ec(glEnableVertexAttribArray(2));
			ec(glEnableVertexAttribArray(3));
			ec(glEnableVertexAttribArray(4));

			ec(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));

			ec(glVertexAttribDivisor(1, 1));
			ec(glVertexAttribDivisor(2, 1));
			ec(glVertexAttribDivisor(3, 1));
			ec(glVertexAttribDivisor(4, 1));

			if (camera)
			{
				lineRenderer->setProjectionViewMatrix(camera->getPerspective() * camera->getView());
				lineRenderer->instanceRender(frameData.lineData.size());
			}
		}
		//frame data cleared when UI render is complete, so we can render for each splitscreen viewport.
	}

	void PlayerPilotAssistUI::handleGameUIRenderComplete()
	{
		frameData.lineData.clear();
	}

}

