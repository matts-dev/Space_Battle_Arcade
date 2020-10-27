#pragma once
#include "SASpaceLevelBase.h"
#include "LevelConfigs/SpaceLevelConfig.h"
//#include "../GameSystems/SAUISystem_Game.h"

namespace SA
{
	class PlayerBase;
	class SpaceLevelConfig;
	class AvoidMesh;
	class Ship;
	class Widget3D_CampaignScreen;
	class CampaignConfig;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Level used to design campaign levels. 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SpaceLevelEditor_Level : public SpaceLevelBase
	{
		using Parent = SpaceLevelBase;
	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		virtual void tick_v(float dt_sec) override;
	private://editor ui
		void renderUI_editor();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int key, int state, int modifier_keys, int scancode);
		void renderUI_levelLoadingSaving();
		void renderUI_campaignDesign();
		void renderUI_gamemodeData();
		void renderUI_avoidMeshPlacement();
		void renderUI_environment();
	private:
		//void handleGameUIRenderDispatch(GameUIRenderData& rd_ui); //appears ui automatically renders for us
		void onActiveLevelConfigSet(const sp<SpaceLevelConfig>& newConfig);
		sp<AvoidMesh> spawnEditorDemoAvoidanceMesh(const std::string& spawnConfigName, const Transform& xform);
		void writeConfigAvoidMeshes();
		void updateLevelData_Star();
		void updateLevelData_Planets();
		void updateLevelData_Nebula();
		void updateLevelData_Carriers(SpaceLevelConfig& newConfig);
		void clearCarriers();
		//void updateLevelData_StarField(); //we're modifying the config directly, no need to od this
		void saveActiveConfig();
		void createNewLevel(const std::string& fileName, const std::string& userFacingName);
		//void saveAvoidanceMesh(size_t index);
	private:
		struct AvoidMeshEditorData
		{
			sp<AvoidMesh> demoMesh = nullptr;
			std::string spawnConfigName = "no spawn config";
		};
		size_t selectedAvoidMesh = 0;
		std::vector<AvoidMeshEditorData> avoidMeshData;
		std::vector<PlanetData> planets_editor;
		std::vector<StarData> localStarData_editor;
		std::vector<NebulaData> nebulaData_editor;
		std::vector<std::vector<sp<Ship>>> teamPlaceholderCarriers;
		sp<SpaceLevelConfig> activeLevelConfig = nullptr;
		sp<CampaignConfig> activeCampaign = nullptr;
		sp<Widget3D_CampaignScreen> campaignWidget = nullptr;

		bool bForceCarrierTakedownGMDataRefresh = false;
	};
}