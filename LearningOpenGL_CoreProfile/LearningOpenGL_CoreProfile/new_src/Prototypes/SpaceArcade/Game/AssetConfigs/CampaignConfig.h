#pragma once
#include "SAConfigBase.h"
#include<cstdint>

namespace SA
{
	class SpaceLevelConfig;

	constexpr size_t MaxLevelTiers = 10;

	class CampaignConfig : public ConfigBase
	{
		using Parent = ConfigBase;
		friend class SpaceArcadeCheatSystem;
	public:
		struct LevelData
		{
			std::string name;
			size_t tier = 0; //the horizontal level this should appear as (levels with same tier will be stacked ontop of each other)
			std::vector<size_t> outGoingPathIndices;
			int64_t optional_defaultPlanetIdx = -1; //leave null to not use one of the default planets
			float optional_ui_planetSizeFactor = 1.f; //valid on range [0.1,10]
			sp<SpaceLevelConfig> spaceLevelConfig;
		};
	protected:
		virtual void postConstruct() override;
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;
		std::string getIndexedName() const;
	public:
		/**Adding support for multiple campaigns in a mod, even though this may not be exposed to UI*/
		void setCampaignIndex(size_t newIndex);
		void requestSave();
		const std::vector<LevelData>& getLevels() { return levels; }
		virtual std::string getRepresentativeFilePath() override;
	private:
		void createTemplateCampaignForJsonSerialization();
	public://serialized fields
		//WARNING: refactoring these names will influence the out put json!
		std::string userFacingName = "Campaign";
		std::vector<LevelData> levels;
	private:
		size_t campaignIndex = 0;
	};
}