#include "CampaignConfig.h"
#include "JsonUtils.h"

namespace SA
{

	void CampaignConfig::postConstruct()
	{
		Parent::postConstruct();
		name = "CampaignConfig";
	}

	void CampaignConfig::onSerialize(json& outData)
	{
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// WARNING:
		//
		// this code path hasn't been tested very much as most levels are contructed directly in the json
		// so the serialization code may have issues, 
		// if using this as an exapmle keep that in mind.
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (levels.size() == 0)
		{
			//create a dummy level t fill out json template
			levels.push_back(LevelData{});
			levels.back().outGoingPathIndices.push_back(0);//create path to itself to fill out template
		}

		json levelArray;
		for (const LevelData& level : levels) //warning refactoring the symbol "level" here will cause changes in serialized json
		{
			json levelJson;

			//NOTE: this means on de-serialization the out symbol will need to be defined with the same name as the object in the for loop (ie level.name);
			levelJson[SYMBOL_TO_STR(level.name)] = level.name;
			levelJson[SYMBOL_TO_STR(level.tier)] = level.tier;

			levelJson[SYMBOL_TO_STR(level.outGoingPathIndices)] = json{}; //start an array
			for (size_t outIndex : level.outGoingPathIndices)
			{
				levelJson[SYMBOL_TO_STR(level.outGoingPathIndices)].push_back(outIndex);
			}

			//the order of this array is very important as level contain indices this array
			levelArray.push_back(levelJson);
		}

		json campaignData =
		{
			{SYMBOL_TO_STR(userFacingName), userFacingName},
			{SYMBOL_TO_STR(levels), levelArray}
		};

		outData.push_back({ getName(), campaignData });
	}

	void CampaignConfig::onDeserialize(const json& inData)
	{
		if(JsonUtils::has(inData, name))
		{
			const json& campaignJson = inData[name];
			if (!campaignJson.is_null())
			{
				READ_JSON_STRING(userFacingName, campaignJson);

				if (JsonUtils::hasArray(campaignJson, SYMBOL_TO_STR(levels)))
				{
					levels.clear(); //clear our levels in memory

					json levelArray = campaignJson[SYMBOL_TO_STR(levels)];
					levelArray.size();
					for (size_t levelIdx = 0; levelIdx < levelArray.size(); ++levelIdx)
					{
						//make a level in memory
						levels.push_back(LevelData{});
						LevelData& level = levels.back();	//WARNING: this must be named "level" to make serialization code as it is used as part of symbol to string translation

						//read from the json
						const json& levelJson = levelArray[levelIdx];
						READ_JSON_STRING(level.name, levelJson);
						READ_JSON_INT(level.tier, levelJson);

						////////////////////////////////////////////////////////
						// read outgoing levels
						////////////////////////////////////////////////////////
						if (JsonUtils::hasArray(levelJson, SYMBOL_TO_STR(level.outGoingPathIndices)))
						{
							const json outgoingPathArrayJson = levelJson[SYMBOL_TO_STR(level.outGoingPathIndices)];

							for (size_t pathIdx = 0; pathIdx < outgoingPathArrayJson.size(); ++pathIdx)
							{
								level.outGoingPathIndices.push_back(size_t(outgoingPathArrayJson[pathIdx]));
							}
						}
					}
				}
			}
		}
	}

	std::string CampaignConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/Campaigns/") + getIndexedName() + std::string(".json");
	}

	std::string CampaignConfig::getIndexedName() const
	{
		return "CampaignConfig_" + std::to_string(campaignIndex);
	}

	void CampaignConfig::setCampaignIndex(size_t newIndex)
	{
		campaignIndex = newIndex;
		//name = getIndexedName();
	}

	void CampaignConfig::requestSave()
	{
		save();
	}

}

