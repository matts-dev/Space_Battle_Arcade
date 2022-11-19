#pragma once
#include <map>

#include<cstdint>

#include "GameFramework/SALevel.h"

namespace SA
{
	class RenderModelEntity;
	class Shader;
	class PlayerBase;
	class PrimitiveShapeRenderer;

	struct Projectile;
	

	class ProjectileEditor_Level : public LevelBase
	{
		enum SimulationState : int
		{
			COLLISION = 0,
			DELTA_TIME
		};
	public:
		virtual bool isEditorLevel() override { return true; }

	private:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void tick_v(float dt_sec) override;

		virtual void postConstruct() override;

		void handleUIFrameStarted();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int /*key*/, int /*state*/, int /*modifier_keys*/, int /*scancode*/);
		void handleMouseButton(int /*button*/, int /*state*/, int /*modifier_keys*/);

		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		void renderCollisionBoxConfiguration(float dt_sec, const glm::mat4& view, const glm::mat4& projection);
		void renderDeltaTimeSimulation(float dt_sec, const glm::mat4& view, const glm::mat4& projection);

	private:
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		// UI MOODEL ADJUSTMENTS
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		glm::vec3 emissiveColor = glm::vec3(0.8f, 0.8f, 0);


		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		// UI COLLISION COLLISION CONFIGURATION 
		/////////////////////////////////////////////////////////////////////////////////////////////////////////

		glm::vec3 collisionBoxColor{ 0,0,1 };

		//imgui's usage of raw ints and "strict aliasing" problems with pointer conversions means the easiest solution
		//is to store this as an int.
		int simulationState = static_cast<int>(SimulationState::COLLISION);
		bool bRenderModelTipOffset_collision = false;
		bool bRenderCollisionBox_collision = false;
		bool bRenderCollisionCenterOffset_collision = false;
		bool bRenderCollisionBoxScaleup_collision = false;
		/*scale the entire projectile, collision included*/
		glm::vec3 projectileScale_collision = glm::vec3(1.f, 1.f, 1.f);				

		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		// UI DT SIMULATION
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		int dtSimulatedFrame = 0;
		glm::vec3 simulatedDeltaTime_dtsimulation = glm::vec3(1 / 60.f);
		glm::vec3 projectileStart_dtsimulation = glm::vec3(0, 0, 0);
		glm::vec3 fireRotation_dtsimulation = glm::vec3(0, 0, 0);
		float projectileSpeed_dtsimulation = 100.f;
		bool bShowCollisionBox_dtsimluation = true;

		sp<RenderModelEntity> projectile;
		std::tuple<glm::vec3, glm::vec3> modelAABB;

		sp<Shader> forwardShaded_EmissiveModelShader;

		sp<PrimitiveShapeRenderer> shapeRenderer;
	};
}
