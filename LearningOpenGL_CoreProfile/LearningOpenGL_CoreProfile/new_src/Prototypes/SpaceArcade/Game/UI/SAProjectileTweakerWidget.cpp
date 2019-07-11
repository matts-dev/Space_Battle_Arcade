#include "SAProjectileTweakerWidget.h"
#include "../SAModSystem.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../SpaceArcade.h"

namespace SA
{
	void ProjectileTweakerWidget::renderInCurrentUIWindow()
	{
		ImGui::Text("Projectile Tweaker");
		ImGui::Dummy(ImVec2(0, 20.f));
		if (ImGui::CollapsingHeader("PROJECTILE LOADING/SAVING", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static int selectedLoadIdx = -1;
			int curConfigIdx = 0;

			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
			const std::map<std::string, sp<ProjectileConfig>>& projectileConfigs = activeMod->getProjectileConfigs();
			//for (const auto& kvPair : projectileConfigs)
			//{
			//	if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedLoadIdx))
			//	{
			//		activeConfig = kvPair.second;
			//		selectedLoadIdx = curConfigIdx;
			//	}
			//	++curConfigIdx;
			//}
		}
	}
}

