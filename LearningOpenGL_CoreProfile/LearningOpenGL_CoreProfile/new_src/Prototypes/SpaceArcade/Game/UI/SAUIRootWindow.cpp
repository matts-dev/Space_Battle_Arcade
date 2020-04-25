#include "SAUIRootWindow.h"
#include "../SpaceArcade.h"
#include "../SAUISystem.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../Levels/BasicTestSpaceLevel.h"
#include "../Levels/ProjectileEditor_Level.h"
#include "../Levels/ModelConfigurerEditor_Level.h"
#include "../SAModSystem.h"
#include<cstdio>
#include "../../GameFramework/SACrossPlatformUtils.h"
#include "../Levels/StressTestLevel.h"

namespace SA
{
	void UIRootWindow::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		const sp<UISystem_Editor>& UI_Sys = game.getUISystem();
		
		UI_Sys->onUIFrameStarted.addWeakObj(sp_this(), &UIRootWindow::handleUIFrameStarted);

	}

	void UIRootWindow::handleUIFrameStarted()
	{
		if (bInUIMode) //NO_UI could be a state, but dev console could be up and that is technically UI
		{
			switch (menuState)
			{
				case UIMenuState::NO_MENU:
					break;
				case UIMenuState::MAIN_MENU:
					buildMainMenu();
					break;
				case UIMenuState::DEV_MENU:
					buildDevMenu();
					break;
				case UIMenuState::MOD_MENU:
					buildModMenu();
					break;
				case UIMenuState::SETTINGS_MENU:
					buildSettingsMenu();
					break;
				default:
					break;
			}
		}
		
		bool bDebugOverlapShouldDisplay = bFPSOverlay /*|| bOtherOverlay*/;
		if (bDebugOverlapShouldDisplay)
		{
			//begin debug stats
			ImGui::SetNextWindowPos(ImVec2{ 0, 0 });
			//ImGui::SetNextWindowSize({ 300,300 });
			ImGuiWindowFlags flags = 0;
			flags |= ImGuiWindowFlags_NoMove;
			flags |= ImGuiWindowFlags_NoResize;
			flags |= ImGuiWindowFlags_NoCollapse;
			ImGui::Begin("Dev Stats!", nullptr, flags);
			{
				if (bFPSOverlay)
				{
					ImGui::Text("%.3f ms | (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				}
			}
			ImGui::End();
		}
	}

	void UIRootWindow::buildMainMenu()
	{
		ImGui::SetNextWindowPosCenter();
		ImGui::SetNextWindowSize({ 300,300 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Space Arcade!", nullptr, flags);
		{
			if (ImGui::Button("Mod Selection"))
			{
				menuState = UIMenuState::MOD_MENU;
			}
			if (ImGui::Button("Settings"))
			{
				menuState = UIMenuState::SETTINGS_MENU;
			}
			if (ImGui::Button("Dev Menu"))
			{
				menuState = UIMenuState::DEV_MENU;
			}
		}
		ImGui::End();
	}

	void UIRootWindow::buildDevMenu()
	{
		ImGui::SetNextWindowPosCenter();
		ImGui::SetNextWindowSize({ 300,300 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Developer Menu!", nullptr, flags);
			if (ImGui::Button("Load Basic Test Level"))
			{
				sp<LevelBase> startupLevel = new_sp<BasicTestSpaceLevel>();
				SpaceArcade::get().getLevelSystem().loadLevel(startupLevel);
			}
			if (ImGui::Button("Load Stress Test Level"))
			{
				sp<LevelBase> startupLevel = new_sp<StressTestLevel>();
				SpaceArcade::get().getLevelSystem().loadLevel(startupLevel);
			}
			if (ImGui::Button("Model Editor"))
			{
				sp<LevelBase> modelEditor = new_sp<ModelConfigurerEditor_Level>();
				SpaceArcade::get().getLevelSystem().loadLevel(modelEditor);
			}
			if (ImGui::Button("Projectile Editor"))
			{
				sp<LevelBase> projectileEditor = new_sp<ProjectileEditor_Level>();
				SpaceArcade::get().getLevelSystem().loadLevel(projectileEditor);
			}
			if (ImGui::Button("Back to Main Menu"))
			{
				menuState = UIMenuState::MAIN_MENU;
			}
		ImGui::End();
	}

	void UIRootWindow::buildModMenu()
	{
		const float windowHeight = 300;

		ImGui::SetNextWindowPosCenter();
		ImGui::SetNextWindowSize({ 300, windowHeight });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Mod Selector!", nullptr, flags);
		{
			ImGui::Text("Available Mods");
			ImGui::Separator();

			static int modSelection = -1;

			const sp<ModSystem>& modSystem = SpaceArcade::get().getModSystem();
			const sp<Mod>& activeMod = modSystem->getActiveMod();

			///////////////////////////////////////////////////////////////////////
			// Mod List
			///////////////////////////////////////////////////////////////////////
			if (modSystem->getMods().size() == 0)
			{
				ImGui::Text("No Mods Available");
			}
			else
			{
				int currentModIdx = 0;
				static char modNameBuffer[MAX_MOD_NAME_LENGTH + 1];
				for (const sp<Mod>& mod : modSystem->getMods())
				{
					if (mod == activeMod){modSelection = currentModIdx;}

					std::string modName = mod->getModName();
					snprintf(modNameBuffer, MAX_MOD_NAME_LENGTH, "%d : %s", currentModIdx, modName.c_str());

					if (ImGui::Selectable(modNameBuffer, currentModIdx == modSelection))
					{
						modSelection = currentModIdx;
						modSystem->setActiveMod(modName);
					}

					++currentModIdx;
				}
			}

			ImGui::Dummy(ImVec2(0, 25));
			ImGui::Separator();
			///////////////////////////////////////////////////////////////////////
			// New Mod Button
			///////////////////////////////////////////////////////////////////////
			static char newModNameBuffer[MAX_MOD_NAME_LENGTH + 1];
			ImGui::InputText("New Mod Name", newModNameBuffer, MAX_MOD_NAME_LENGTH, ImGuiInputTextFlags_CharsNoBlank);

			if (ImGui::Button("Create New Mod"))
			{
				std::string newModName(newModNameBuffer);
				if (modSystem->createNewMod(newModName))
				{
					//clear buffer
					newModNameBuffer[0] = 0;
				}
			}

			///////////////////////////////////////////////////////////////////////
			// Delete Button
			///////////////////////////////////////////////////////////////////////
			bool bDeleteEnabled = activeMod != nullptr && activeMod->isDeletable();
			if (bDeleteEnabled)
			{
				ImGui::SameLine(); //only call same line if we're actually inlining a widget
				if (ImGui::Button("Delete Mod"))
				{
					ImGui::OpenPopup("DeleteModPopup");
				}
			}
			else if(activeMod)
			{
				ImGui::SameLine(); //only call same line if we're actually in lining a widget
				ImGui::Text("This mod isn't deletable");
			}

			///////////////////////////////////////////////////////////////////////
			// Delete Popup
			///////////////////////////////////////////////////////////////////////
			if (ImGui::BeginPopupModal("DeleteModPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("WARNING: this operation is irreversible!");
				ImGui::Text("Do you really want to delete this mod?");

				if (ImGui::Button("DELETE", ImVec2(120, 0))) 
				{ 
					modSystem->deleteMod(activeMod->getModName());
					ImGui::CloseCurrentPopup(); 
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("CANCEL", ImVec2(120, 0)))
				{ 
					ImGui::CloseCurrentPopup(); 
				}
				ImGui::EndPopup();
			}

			///////////////////////////////////////////////////////////////////////
			// Back To Main Menu
			///////////////////////////////////////////////////////////////////////
			ImGui::Dummy(ImVec2(0, 25)); 
			ImGui::Separator();
			if (ImGui::Button("Back to Main Menu"))
			{
				menuState = UIMenuState::MAIN_MENU;
			}
		}
		ImGui::End();
	}

	void UIRootWindow::buildSettingsMenu()
	{
		ImGui::SetNextWindowPosCenter();
		ImGui::SetNextWindowSize({ 300,300 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Settings", nullptr, flags);
		{
			ImGui::Checkbox("Display FPS", &bFPSOverlay);
			if (ImGui::Button("Back"))
			{
				menuState = UIMenuState::MAIN_MENU;
			}
		}
		ImGui::End();
	}

	void UIRootWindow::tick(float dt_sec)
	{

	}
}

