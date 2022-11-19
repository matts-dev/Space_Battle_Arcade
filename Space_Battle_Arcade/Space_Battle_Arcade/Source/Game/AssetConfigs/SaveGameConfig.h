#pragma once
#include "Game/AssetConfigs/SAConfigBase.h"

namespace SA
{
	class SaveGameConfig : public ConfigBase
	{
		using Parent = ConfigBase;
		friend class SpaceArcadeCheatSystem;
	public:
		struct CampaignData
		{
			std::string campaignName;
			std::vector<size_t> completedLevels;
		};
	protected:
		virtual void postConstruct() override;
	public:
		void applyDemoDataIfEmpty();
		void requestSave();
	public:
		virtual std::string getRepresentativeFilePath() override;
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;
		void addCampaign(const std::string& campaignName);
		const CampaignData* findCampaignByName(const std::string& campaignName) const;
		CampaignData* findCampaignByName_Mutable(const std::string& campaignName);
	private:
		std::vector<CampaignData> campaignData;
	};

}