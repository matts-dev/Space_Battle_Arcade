#include "ModelConfigurerEditor_Level.h"
#include "..\SpaceArcade.h"
#include "..\SAUISubsystem.h"
#include "..\..\..\..\..\Libraries\imgui.1.69.gl\imgui.h"
#include "..\..\Rendering\Camera\SACameraBase.h"
#include "..\..\GameFramework\SAPlayerBase.h"
#include "..\..\GameFramework\Input\SAInput.h"
#include "..\..\GameFramework\SAPlayerSubsystem.h"

namespace SA
{
	void ModelConfigurerEditor_Level::startLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getUISubsystem()->onUIFrameStarted.addStrongObj(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);

		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePlayerCreated);
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}

	}

	void ModelConfigurerEditor_Level::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getUISubsystem()->onUIFrameStarted.removeStrong(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);
		
	}

	void ModelConfigurerEditor_Level::handleUIFrameStarted()
	{
		ImGui::SetNextWindowPos(ImVec2{ 25, 25 });
		//ImGui::SetNextWindowSize(ImVec2{ 400, 600 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		//flags |= ImGuiWindowFlags_NoResize;
		//flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Model Editor!", nullptr, flags);
		{
			ImGui::Text("Press T to toggle between mouse and camera.");
			ImGui::Text("Loaded Entity: "); ImGui::SameLine(); ImGui::Text("None");
			ImGui::Separator();

			if (ImGui::CollapsingHeader("ENTITY LOADING/SAVING"))
			{
				ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;
				ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, { 0.1f, 0.1f, 0.1f, 1.f });
				ImGui::BeginChild("SavedConfirgurationsMenu", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 100), false, file_load_wndflags);
				{
					for (int test = 0; test < 5; ++test)
					{
						ImGui::Text("File name, not path");
					}
				}
				ImGui::EndChild();
				ImGui::PopStyleColor();

				ImGui::SameLine();
				ImGui::BeginChild("model interaction buttons", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 100), false, 0);
				{
					ImGui::Button("Actionable!");
					ImGui::Button("Actionable!");
					ImGui::Button("Actionable!");
				}
				ImGui::EndChild();

				ImGui::BeginChild("add new model");
				{
					ImGui::Separator();
					static char filepath_buffer[512 + 1];
					ImGui::InputText("New Entity Name", filepath_buffer, 512, 0);

					ImGui::InputText("3D model Filepath", filepath_buffer, 512, 0);
					ImGui::Text("File path should be relative to mod");
					ImGui::Text("eg /mods/basegame/figher.obj");
					ImGui::Button("Add Entity");
					ImGui::Separator();
				}
				ImGui::EndChild();
			}
			if (ImGui::CollapsingHeader("COLLISION"))
			{
				{
					ImGui::Separator();
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Separator();
				}
			}
			if (ImGui::CollapsingHeader("FIRING PROJECTILES"))
			{
				{
					ImGui::Separator();
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Separator();
				}
			}
			if (ImGui::CollapsingHeader("COLOR"))
			{
				{
					ImGui::Separator();
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Separator();
				}
			}
			if (ImGui::CollapsingHeader("TEAM"))
			{
				{
					ImGui::Separator();
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Text("place-holder");
					ImGui::Separator();
				}
			}
		}
		ImGui::End();
	}

	void ModelConfigurerEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleKey);
		player->getInput().onButton.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleMouseButton);
		if (const sp<CameraBase>& camera = player->getCamera())
		{
			//configure camera to look at model
			//camera->setPosition({ 5, 0, 0 });
			//camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
		}
	}

	void ModelConfigurerEditor_Level::handleKey(int key, int state, int modifier_keys, int scancode)
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

	void ModelConfigurerEditor_Level::handleMouseButton(int button, int state, int modifier_keys)
	{

	}

}

