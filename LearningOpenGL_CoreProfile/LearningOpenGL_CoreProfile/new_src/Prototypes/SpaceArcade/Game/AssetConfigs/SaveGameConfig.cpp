#include "SaveGameConfig.h"
#include "JsonUtils.h"
#include "../GameSystems/SAModSystem.h"
#include "../SpaceArcade.h"

namespace SA
{
	void SaveGameConfig::postConstruct()
	{
		Parent::postConstruct();
		fileName = SYMBOL_TO_STR(SaveGameConfig);
	}

	void SaveGameConfig::applyDemoDataIfEmpty()
	{
		//generate a test template if empty
		if (bool bEmpty = campaignData.size() == 0)
		{
			if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
			{
				owningModDir = activeMod->getModDirectoryPath();

				for (size_t campaignIdx = 0; campaignIdx < 2; ++campaignIdx)
				{
					campaignData.push_back(CampaignData{});
					CampaignData& campaign = campaignData.back();

					campaign.campaignName = std::string("some/file/path/campaign_name.json");
					for (size_t completedLevelIdx = 0; completedLevelIdx < 3; ++completedLevelIdx)
					{
						campaign.completedLevels.push_back(2 * completedLevelIdx); //*2 just to show that levels may be sparse in template
					}
				}
			}
		}
	}

	std::string SaveGameConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("GameSaves/") + fileName + std::string(".json");
	}

	void SaveGameConfig::onSerialize(json& outData)
	{
		Parent::onSerialize(outData);

		json json_campaigns_array;
		for (CampaignData& campaign : campaignData) //note campaign must be used in deserialization 
		{
			json json_array_levelData;
			for (size_t completedLevel : campaign.completedLevels)
			{
				json_array_levelData.push_back(completedLevel);
			}

			json json_campaign;
			JSON_WRITE(campaign.campaignName, json_campaign);
			json_campaign[SYMBOL_TO_STR(campaign.completedLevels)] = json_array_levelData;

			json_campaigns_array.push_back(json_campaign);
		}

		json json_saveGameMap;
		json_saveGameMap[SYMBOL_TO_STR(campaignData)] = json_campaigns_array;

		//make entry for this subclass in entire json
		outData[fileName] = json_saveGameMap;
	}

	void SaveGameConfig::onDeserialize(const json& inData)
	{
		Parent::onDeserialize(inData);
		
		if (JsonUtils::has(inData, fileName))
		{
			json json_SaveGameConfig = inData[fileName];

			if (JsonUtils::hasArray(json_SaveGameConfig, SYMBOL_TO_STR(campaignData)))
			{
				campaignData.clear(); //clear any previous data, just to be safe (not necessarily expected)

				json json_campaignArray = json_SaveGameConfig[SYMBOL_TO_STR(campaignData)];
				for (size_t campaignIdx = 0; campaignIdx < json_campaignArray.size(); ++campaignIdx)
				{
					campaignData.push_back(CampaignData{});
					CampaignData& campaign = campaignData.back();

					json json_campaignData = json_campaignArray[campaignIdx];
					READ_JSON_STRING_OPTIONAL(campaign.campaignName, json_campaignData);

					if (JsonUtils::hasArray(json_campaignData, SYMBOL_TO_STR(campaign.completedLevels)))
					{
						campaign.completedLevels.clear(); //clear any previous data before we add to it, just to be safe

						json json_completedLevelArray = json_campaignData[SYMBOL_TO_STR(campaign.completedLevels)];
						for (size_t completedLevelIdx = 0; completedLevelIdx < json_completedLevelArray.size(); ++completedLevelIdx)
						{
							size_t completedLevel = json_completedLevelArray[completedLevelIdx];
							campaign.completedLevels.push_back(completedLevel);
						}
					}
				}
			}
		}
	}

	void SaveGameConfig::addCampaign(const std::string& campaignName)
	{
		// In this context the campaign name is its complete file path
		if (findCampaignByName(campaignName) != nullptr)
		{
			STOP_DEBUGGER_HERE();
			return; //we already have this campaign, this shouldn't happen
		}

		CampaignData newCampaign;
		newCampaign.campaignName = campaignName;
		campaignData.push_back(newCampaign);
	}

	const SaveGameConfig::CampaignData* SaveGameConfig::findCampaignByName(const std::string& campaignName) const
	{
		for (const CampaignData& campaign : campaignData)
		{
			if (campaign.campaignName == campaignName)
			{
				return &campaign;
			}
		}

		return nullptr;
	}

	SA::SaveGameConfig::CampaignData* SaveGameConfig::findCampaignByName_Mutable(const std::string& campaignName)
	{
		return const_cast<CampaignData*>( static_cast<const SaveGameConfig*>(this)->findCampaignByName(campaignName));
	}

}

