#include "SAProjectileTweakerWidget.h"
#include "../../GameSystems/SAModSystem.h"
#include "../../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../../SpaceArcade.h"
#include "../../AssetConfigs/SAProjectileConfig.h"

namespace SA
{
	void ProjectileTweakerWidget::renderInCurrentUIWindow()
	{
		ImGui::Text("Projectile Tweaker");
		ImGui::Dummy(ImVec2(0, 20.f));

		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (!activeMod)
		{
			ImGui::Text("No active mode");
			return;
		}

		static int selectedLoadIdx = -1;
		if (ImGui::CollapsingHeader("PROJECTILE LOADING/SAVING", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int curConfigIdx = 0;

			ImGui::Text("Files:");
			ImGui::Separator();
			ImGui::Dummy({ 0.0, 5.0f });
			const std::map<std::string, sp<ProjectileConfig>>& projectileConfigs = activeMod->getProjectileConfigs();
			for (const auto& kvPair : projectileConfigs)
			{
				if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedLoadIdx))
				{
					activeConfig = kvPair.second;
					selectedLoadIdx = curConfigIdx;
				}
				++curConfigIdx;
			}

			ImGui::Dummy({ 0.0, 5.0f });
			ImGui::Separator();

			static char newNameBuffer[1024];
			ImGui::InputTextWithHint("New Projectile Config Name", "eg. FighterLaser", newNameBuffer, sizeof(newNameBuffer) / sizeof(char)); //I realize /sizeof(char) is redundant but I have my reasons

			bool bShowAdd = newNameBuffer[0] != 0;
			if (bShowAdd)
			{
				if (ImGui::Button("Add"))
				{
					createNewProjectileConfig(newNameBuffer);
					newNameBuffer[0] = 0;
				}
			}
			if (selectedLoadIdx != -1 && activeConfig)
			{
				if (bShowAdd) //inline the delete button
				{
					ImGui::SameLine(); ImGui::Dummy({ 10, 0 }); ImGui::SameLine();
				}
				if (ImGui::Button("Delete Selected"))
				{
					ImGui::OpenPopup("DeleteProjectileConfigPopup");
				}
			}

			if (ImGui::BeginPopupModal("DeleteProjectileConfigPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (activeConfig)
				{
					ImGui::Text("DELETE:"); ImGui::SameLine(); ImGui::Text(activeConfig->getName().c_str());
					ImGui::Text("WARNING: this operation is irreversible!");
					ImGui::Text("Do you really want to delete this projectile config?");

					if (ImGui::Button("DELETE", ImVec2(120, 0)))
					{
						if (activeMod)
						{
							activeMod->deleteProjectileConfig(activeConfig);
							activeConfig = nullptr;
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
				}
				if (ImGui::Button("CANCEL", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Dummy({ 0, 25.0f }); //pad end of collapsible header
		}
		if (ImGui::CollapsingHeader("PROJECTILE VALUES", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (selectedLoadIdx != -1 && activeConfig)
			{
				ImGui::InputFloat("speed", &activeConfig->speed);
				ImGui::InputFloat("lifetimeSec", &activeConfig->lifetimeSecs);
				ImGui::InputFloat3("color", &activeConfig->color.r);

				ImGui::Dummy({ 0, 25.0f });
				if (ImGui::Button("Save"))
				{
					activeConfig->save();
				}
			}
		}
	}

	void ProjectileTweakerWidget::createNewProjectileConfig(const std::string& name)
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			sp<ProjectileConfig> newProjConfig = new_sp<ProjectileConfig>();
			newProjConfig->fileName = name;
			newProjConfig->owningModDir = activeMod->getModDirectoryPath(); //#alternative perhaps should just pass owning mod to ctor and have this taken careof
			newProjConfig->save();

			activeMod->addProjectileConfig(newProjConfig);
			activeConfig = newProjConfig;
		}
	}

}

