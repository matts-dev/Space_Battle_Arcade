#include "ProjectileEditor_Level.h"
#include "..\SpaceArcade.h"
#include "..\SAShip.h"
#include "..\..\GameFramework\RenderModelEntity.h"
#include "..\SAProjectileSubsystem.h"
#include "..\SAUISubsystem.h"
#include "..\..\..\..\..\Libraries\imgui.1.69.gl\imgui.h"
#include "..\..\Rendering\BuiltInShaders.h"
#include "..\..\GameFramework\SAPlayerBase.h"
#include "..\..\GameFramework\SAGameBase.h"
#include "..\..\GameFramework\SAPlayerSubsystem.h"
#include "..\..\GameFramework\Input\SAInput.h"
#include "..\..\Rendering\Camera\SACameraBase.h"
#include "..\..\..\..\Algorithms\SpatialHashing\SHDebugUtils.h"

namespace SA
{

	void ProjectileEditor_Level::startLevel_v()
	{
		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);
		debugLineShader = new_sp<Shader>(SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false);

		SpaceArcade& game = SpaceArcade::get();
		game.getUISubsystem()->onUIFrameStarted.addStrongObj(sp_this(), &ProjectileEditor_Level::handleUIFrameStarted);
		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ProjectileEditor_Level::handlePlayerCreated);
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}

		//configure projectile
		sp<Model3D> projectileModel = game.getModel(game.laserBoltModelKey);

		Transform worldTransform;
		projectile = new_sp<RenderModelEntity>(projectileModel, worldTransform);
	}

	void ProjectileEditor_Level::endLevel_v()
	{
		forwardShaded_EmissiveModelShader = nullptr;
		debugLineShader = nullptr;

		SpaceArcade& game = SpaceArcade::get();

		game.getUISubsystem()->onUIFrameStarted.removeStrong(sp_this(), &ProjectileEditor_Level::handleUIFrameStarted);
 
		//unspawn all projectiles so they're not in the new level
		sp<ProjectileSubsystem> projectileSS = game.getProjectileSS();
		projectileSS->unspawnAllProjectiles();
	}

	void ProjectileEditor_Level::tick(float dt_sec)
	{
		
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
			ImGui::Checkbox("Render Model Tip Offset", &bRenderModelTipOffset);
			ImGui::Checkbox("Render Collision Box", &bRenderCollisionBox);
			ImGui::Checkbox("Render Collision Center Offset",  &bRenderCollisionCenterOffset);
			ImGui::Checkbox("Render Collision Box Scale Up",  &bRenderCollisionBoxScaleup);
			ImGui::SliderFloat("projectile scale  ", &projectileScale, 0.1f, 10.f);
			ImGui::SliderFloat("Simulated dt_sec", &simulatedDeltaTime, 0.f, 10.f);
		ImGui::End();
	}

	void ProjectileEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &ProjectileEditor_Level::handleKey);
		player->getInput().onButton.addWeakObj(sp_this(), &ProjectileEditor_Level::handleButton);
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

	void ProjectileEditor_Level::handleButton(int button, int state, int modifier_keys)
	{

	}

	void ProjectileEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		if (projectile)
		{
			glm::mat4 model = projectile->getTransform().getModelMatrix();
			forwardShaded_EmissiveModelShader->use();
			forwardShaded_EmissiveModelShader->setUniform3f("lightColor", glm::vec3(0.8f, 0.8f, 0));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShaded_EmissiveModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			projectile->getModel()->draw(*forwardShaded_EmissiveModelShader);
		}


	}
}

