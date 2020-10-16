#pragma once
#include "SASpaceLevelBase.h"
#include "LevelConfigs/SpaceLevelConfig.h"

namespace SA
{
	class PlayerBase;
	class SpaceLevelConfig;
	class AvoidMesh;

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
	private://editor ui
		void renderUI_editor();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int key, int state, int modifier_keys, int scancode);
		void renderUI_levelLoadingSaving();
		void renderUI_avoidMeshPlacement();
		void renderUI_environment();
	private:
		void onActiveLevelConfigSet(const sp<SpaceLevelConfig>& newConfig);
		sp<AvoidMesh> spawnEditorDemoAvoidanceMesh(const std::string& spawnConfigName, const Transform& xform);
		void updateConfigAvoidMeshes();
		void updateLevelData_Star();
		void updateLevelData_Planets();
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
		sp<SpaceLevelConfig> activeLevelConfig = nullptr;
	};
}