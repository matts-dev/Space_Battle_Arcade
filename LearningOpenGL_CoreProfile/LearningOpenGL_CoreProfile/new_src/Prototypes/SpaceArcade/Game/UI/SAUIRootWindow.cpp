#include "SAUIRootWindow.h"
#include "../SpaceArcade.h"
#include "../SAUISubsystem.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../Levels/BasicTestSpaceLevel.h"
#include "../Levels/ProjectileEditor_Level.h"

namespace SA
{

	void UIRootWindow::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		const sp<UISubsystem>& UI_SS = game.getUISubsystem();
		
		UI_SS->onUIFrameStarted.addWeakObj(sp_this(), &UIRootWindow::handleUIFrameStarted);

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
				default:
					break;
			}
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
			if (ImGui::Button("Dev Menu"))
			{
				menuState = UIMenuState::DEV_MENU;
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
				SpaceArcade::get().getLevelSubsystem().loadLevel(startupLevel);
			}
			if (ImGui::Button("Projectile Editor"))
			{
				sp<LevelBase> projectileEditor = new_sp<ProjectileEditor_Level>();
				SpaceArcade::get().getLevelSubsystem().loadLevel(projectileEditor);
			}
			if (ImGui::Button("Back to Main Menu"))
			{
				menuState = UIMenuState::MAIN_MENU;
			}
		ImGui::End();
	}

	void UIRootWindow::tick(float dt_sec)
	{

	}
}

