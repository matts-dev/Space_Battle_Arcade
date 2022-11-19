#include "Game/GameSystems/SAProjectileSystem.h"
#include "Game/GameSystems/SAUISystem_Editor.h"
#include "Game/Levels/ProjectileEditor_Level.h"
#include "Game/SAPrimitiveShapeRenderer.h"
#include "Game/SAShip.h"
#include "Game/SpaceArcade.h"
#include "GameFramework/Input/SAInput.h"
#include "GameFramework/RenderModelEntity.h"
#include "GameFramework/SAAssetSystem.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAPlayerBase.h"
#include "GameFramework/SAPlayerSystem.h"
#include "GameFramework/SAWindowSystem.h"
#include "Libraries/imgui.1.69.gl/imgui.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SHDebugUtils.h"
#include "Rendering/BuiltInShaders.h"
#include "Rendering/Camera/SACameraBase.h"
#include "Rendering/Camera/SAQuaternionCamera.h"

namespace SA
{

	void ProjectileEditor_Level::startLevel_v()
	{
		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);

		shapeRenderer = new_sp<PrimitiveShapeRenderer>();

		SpaceArcade& game = SpaceArcade::get();
		game.getEditorUISystem()->onUIFrameStarted.addStrongObj(sp_this(), &ProjectileEditor_Level::handleUIFrameStarted);
		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ProjectileEditor_Level::handlePlayerCreated);
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}

		//configure projectile
		sp<Model3D> projectileModel = game.getAssetSystem().getModel(game.URLs.laserURL); 

		Transform worldTransform;

		projectile = new_sp<RenderModelEntity>(projectileModel, worldTransform);
		if (const sp<const Model3D>& model = projectile->getModel())
		{
			modelAABB = model->getAABB();
		}
	}

	void ProjectileEditor_Level::endLevel_v()
	{
		forwardShaded_EmissiveModelShader = nullptr;

		SpaceArcade& game = SpaceArcade::get();

		game.getEditorUISystem()->onUIFrameStarted.removeStrong(sp_this(), &ProjectileEditor_Level::handleUIFrameStarted);
 
		//unspawn all projectiles so they're not in the new level
		sp<ProjectileSystem> projectileSys = game.getProjectileSystem();
		projectileSys->unspawnAllProjectiles();
	}

	void ProjectileEditor_Level::tick_v(float dt_sec)
	{
		LevelBase::tick_v(dt_sec);
	}

	void ProjectileEditor_Level::postConstruct()
	{
	}

	void ProjectileEditor_Level::handleUIFrameStarted()
	{
		SpaceArcade& game = SpaceArcade::get();

		ImGui::SetNextWindowPos(ImVec2{ 25, 25 });
		//ImGui::SetNextWindowSize(ImVec2{ 350, 300 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Projectile Editor!", nullptr, flags);
			ImGui::Text("Press T to toggle between mouse and camera.");

			//static casts on enums because ImGUI api expects pointers to ints
			ImGui::RadioButton("Collision Configuration", &simulationState, (int)SimulationState::COLLISION); ImGui::SameLine();
			ImGui::RadioButton("Delta Time Simulation", &simulationState, (int)SimulationState::DELTA_TIME);
			//ImGui::NewLine();
			ImGui::Separator();

			SimulationState currentState = static_cast<SimulationState>(simulationState);
			if (currentState == SimulationState::COLLISION)
			{
				ImGui::Checkbox("Render Collision Box", &bRenderCollisionBox_collision);
				ImGui::Checkbox("Render Collision Box Scale Up",  &bRenderCollisionBoxScaleup_collision);
				ImGui::Checkbox("Render Model Tip Offset", &bRenderModelTipOffset_collision);
				ImGui::Checkbox("Render Collision Center Offset",  &bRenderCollisionCenterOffset_collision);
				ImGui::SliderFloat3("projectile scale  ", &projectileScale_collision.x, 0.1f, 10.f);
			}
			else if (currentState == SimulationState::DELTA_TIME)
			{
				ImGui::RadioButton("Frame 1", &dtSimulatedFrame, 0); ImGui::SameLine();
				ImGui::RadioButton("Frame 2", &dtSimulatedFrame, 1); ImGui::SameLine();
				ImGui::RadioButton("Frame 3", &dtSimulatedFrame, 2); 

				ImGui::SliderFloat3("Simulated dt_sec", &simulatedDeltaTime_dtsimulation.x, 0.f, 0.5f); 
				ImGui::InputFloat3("projectile start position", &projectileStart_dtsimulation.x);
				ImGui::SliderFloat3("Fire Rotation", &fireRotation_dtsimulation.x, -180.f, 180.f);
				ImGui::SliderFloat("projectile speed", &projectileSpeed_dtsimulation, 0.f, 1000.f);
				ImGui::Checkbox("show collision", &bShowCollisionBox_dtsimluation);
			}
		ImGui::End();
	}

	void ProjectileEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &ProjectileEditor_Level::handleKey);
		player->getInput().onButton.addWeakObj(sp_this(), &ProjectileEditor_Level::handleMouseButton);

		sp<QuaternionCamera> quatCam = new_sp<QuaternionCamera>();
		quatCam->registerToWindowCallbacks_v(GameBase::get().getWindowSystem().getPrimaryWindow());
		player->setCamera(quatCam);

		if (const sp<CameraBase>& camera = player->getCamera())
		{
			camera->setPosition({ 5, 0, 0 });
			camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
		}
	}

	void ProjectileEditor_Level::handleKey(int key, int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			if (key == GLFW_KEY_T)
			{
				const sp<PlayerBase>& zeroPlayer = SpaceArcade::get().getPlayerSystem().getPlayer(0);
				if (const sp<CameraBase>& camera = zeroPlayer->getCamera())
				{
					camera->setCursorMode(!camera->isInCursorMode());
				}
			}
		}
	}

	void ProjectileEditor_Level::handleMouseButton(int button, int state, int modifier_keys)
	{
		
	}

	void ProjectileEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		//render origin red
		mat4 originBoxScaleDown = glm::scale(mat4(1.f), vec3(0.1f, 0.1f, 0.1f));
		//shapeRenderer->renderUnitCube({ originBoxScaleDown, view, projection, {1,0,0} });
		shapeRenderer->renderAxes(mat4(1.f), view, projection);

		SimulationState currentState = static_cast<SimulationState>(simulationState);
		switch (currentState)
		{
			case SimulationState::COLLISION:
				renderCollisionBoxConfiguration(dt_sec, view, projection);
				break;
			case SimulationState::DELTA_TIME:
				renderDeltaTimeSimulation(dt_sec, view, projection);
				break;
			default:
				break;
		}
	}	

	void ProjectileEditor_Level::renderCollisionBoxConfiguration(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		mat4 projectileModelMat = projectile->getTransform().getModelMatrix();
		mat4 collisionBoxModelMat = mat4(1.0f);

		mat4 projScaleMat = glm::scale(mat4(1.f), projectileScale_collision);
		projectileModelMat = projScaleMat;// *projectileModelMat; //projectileModelMat may or maynot be the configuration transform for the projectile, not it's world transform

		//aabb calculations
		const vec3& aabbMin = std::get<0>(modelAABB);
		const vec3& aabbMax = std::get<1>(modelAABB);
		const vec3 aabbDelta = aabbMax - aabbMin;
		const vec3 scaledAABBDelta = aabbDelta * projectileScale_collision;

		//move rightmost tip of model to origin (after scaling)
		const vec3 halfAABB = scaledAABBDelta / vec3(2.0f);
		glm::mat4 offsetTipMat = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, halfAABB.z));


		if (bRenderModelTipOffset_collision)
		{
			projectileModelMat = offsetTipMat * projectileModelMat;
		}

		if (bRenderCollisionBox_collision)
		{
			if (bRenderCollisionCenterOffset_collision)
			{
				collisionBoxModelMat = glm::translate(collisionBoxModelMat, vec3(0, 0, halfAABB.z));
			}
			if (bRenderCollisionBoxScaleup_collision) //MUST COME AFTER CENTER TRANSLATION (ie scale is applied first)
			{
				collisionBoxModelMat = glm::scale(collisionBoxModelMat, scaledAABBDelta);
			}
			shapeRenderer->renderUnitCube({ collisionBoxModelMat , view, projection, collisionBoxColor, GL_LINE, GL_FILL });
		}


		if (projectile)
		{
			forwardShaded_EmissiveModelShader->use();
			forwardShaded_EmissiveModelShader->setUniform3f("lightColor", emissiveColor);
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectileModelMat));
			projectile->getModel()->draw(*forwardShaded_EmissiveModelShader);
		}
	}

	void ProjectileEditor_Level::renderDeltaTimeSimulation(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4; using glm::vec4; using glm::quat;

		vec3 frame0EndPnt;
		vec3 frame1EndPnt;
		vec3 frame2EndPnt;

		//dts.x = frame0, dts.y = frame1, dts.z = frame2
		vec3 dts = simulatedDeltaTime_dtsimulation;
		vec3 start = projectileStart_dtsimulation;
		float speed = projectileSpeed_dtsimulation;
		quat fireRot = glm::angleAxis(glm::radians(fireRotation_dtsimulation.x), vec3(1, 0, 0));
		fireRot = fireRot * glm::angleAxis(glm::radians(fireRotation_dtsimulation.y), vec3(0, 1, 0));
		fireRot = fireRot * glm::angleAxis(glm::radians(fireRotation_dtsimulation.z), vec3(0, 0, 1));
		glm::vec4 direction(0, 0, -1, 0);

		const mat4 fireRotMat = glm::toMat4(fireRot);
		vec3 firDir = glm::normalize(vec3(fireRotMat * direction));

		frame0EndPnt = start + dts.x * (speed * firDir);
		frame1EndPnt = frame0EndPnt + dts.y * (speed * firDir);
		frame2EndPnt = frame1EndPnt + dts.z	* (speed * firDir);

		vec3 frameStart;
		vec3 frameEnd;
		float frameDt;
		switch (dtSimulatedFrame)
		{
			case 0:
				frameStart = start;
				frameEnd = frame0EndPnt;
				frameDt = dts.x;
				break;
			case 1:
				frameStart = frame0EndPnt;
				frameEnd = frame1EndPnt;
				frameDt = dts.y;
				break;
			case 2:
				frameStart = frame1EndPnt;
				frameEnd = frame2EndPnt;
				frameDt = dts.z;
				break;
		}

		mat4 originBoxScaleDown = glm::scale(mat4(1.f), vec3(0.05f, 0.05f, 0.05f));
		mat4 frame0StartPntMat = glm::translate(mat4(1.f), start);
		mat4 frame0EndPntMat = glm::translate(mat4(1.f), frame0EndPnt);
		mat4 frame1EndPntMat = glm::translate(mat4(1.f), frame1EndPnt);
		mat4 frame2EndPntMat = glm::translate(mat4(1.f), frame2EndPnt);
		shapeRenderer->renderUnitCube({ frame0StartPntMat*originBoxScaleDown, view, projection, {0,1,0} });
		shapeRenderer->renderUnitCube({ frame0EndPntMat*originBoxScaleDown, view, projection, {0,1,0} });
		shapeRenderer->renderUnitCube({ frame1EndPntMat*originBoxScaleDown, view, projection, {0,1,0} });
		shapeRenderer->renderUnitCube({ frame2EndPntMat*originBoxScaleDown, view, projection, {0,1,0} });

		if (projectile)
		{
			//support custom model rotation adjustments
			mat4 modelAdjustmentMat = projectile->getTransform().getModelMatrix();

			//there may be some optimizations that can happen here
			glm::vec3 offsetDir = glm::normalize(frameStart - frameEnd);
			float travelDistance = frameDt * speed;
			float offsetLength = (travelDistance) / 2.f; //this avoids 2 sqrt calculations, 1 sqrt for normalization, 1 for length
		
			//aabb calculations
			const vec3& aabbMin = std::get<0>(modelAABB);
			const vec3& aabbMax = std::get<1>(modelAABB);
			const vec3 aabbDelta = aabbMax - aabbMin;

			//need to rotate aabb based on custom rotation adjustments, if user rotates model's
			//x-axis to z axis, we need to make sure the scaling is appropriately applied
			const vec3 rotatedAABB = vec3(vec4(aabbDelta, 0.f) * modelAdjustmentMat);

			//z-axis is the direction of translation in local space
			vec3 stretchScale(1.0f);
			stretchScale.z = travelDistance / rotatedAABB.z;
			mat4 stretchXForm = glm::scale(mat4(1.f), stretchScale);

			//collision box needs to fix model exactly
			vec3 collisionBoxStretchScale = rotatedAABB;
			collisionBoxStretchScale.z = travelDistance;
			mat4 collisionBoxStretchScaleMat = glm::scale(mat4(1.f), collisionBoxStretchScale);

			mat4 offsetXForm = glm::translate(mat4(1.f), vec3(0,0, travelDistance / 2));

			mat4 translateToEndPnt = glm::translate(glm::mat4(1.f), frameEnd);

			mat4 translateEnd_firerot_offset = translateToEndPnt * fireRotMat * offsetXForm;

			mat4 projectileTransform = translateEnd_firerot_offset * stretchXForm * modelAdjustmentMat;
			const mat4& collisionXform = translateEnd_firerot_offset * collisionBoxStretchScaleMat;

			forwardShaded_EmissiveModelShader->use(); 
			forwardShaded_EmissiveModelShader->setUniform3f("lightColor", emissiveColor);
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectileTransform));
			projectile->getModel()->draw(*forwardShaded_EmissiveModelShader);

			if (bShowCollisionBox_dtsimluation)
			{
				shapeRenderer->renderUnitCube({ collisionXform , view, projection, collisionBoxColor, GL_LINE, GL_FILL });
			}
		}
		
	}

}

