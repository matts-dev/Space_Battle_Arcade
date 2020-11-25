#include "SASettingsProfileConfig.h"
#include "JsonUtils.h"

namespace SA
{



	void SettingsProfileConfig::onSerialize(json& outData)
	{
		json settingsData_j =
		{
			{"bEnableDevConsole", bEnableDevConsole},
			{"masterVolume", masterVolume}
		};
		JSON_WRITE(selectedTeamIdx, settingsData_j);
		JSON_WRITE(scalabilitySettings.multiplier_maxSpawnableShips, settingsData_j);
		JSON_WRITE(scalabilitySettings.multiplier_spawnComponentCooldownSec, settingsData_j);

		std::string indexName = getIndexedName();
		outData.push_back({ fileName, settingsData_j});
	}

	void SettingsProfileConfig::onDeserialize(const json& inData)
	{
		std::string indexedName = getIndexedName();

		if (!inData.is_null() && inData.contains(indexedName))
		{
			const json& settingProfileData_j = inData[indexedName];
			if (!settingProfileData_j.is_null())
			{
				if (settingProfileData_j.contains("bEnableDevConsole") && !settingProfileData_j["bEnableDevConsole"].is_null() && settingProfileData_j["bEnableDevConsole"].is_boolean())
				{
					bEnableDevConsole = settingProfileData_j["bEnableDevConsole"];
				}
				if (settingProfileData_j.contains("masterVolume") && !settingProfileData_j["masterVolume"].is_null() && settingProfileData_j["masterVolume"].is_number_float())
				{
					masterVolume = settingProfileData_j["masterVolume"];
				}

				READ_JSON_INT_OPTIONAL(selectedTeamIdx, settingProfileData_j);
				READ_JSON_FLOAT_OPTIONAL(scalabilitySettings.multiplier_maxSpawnableShips, settingProfileData_j);
				READ_JSON_FLOAT_OPTIONAL(scalabilitySettings.multiplier_spawnComponentCooldownSec, settingProfileData_j);
			}
		}
	}

	std::string SettingsProfileConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/Settings/") + getIndexedName() + std::string(".json");
	}

	std::string SettingsProfileConfig::getIndexedName() const
	{
		return "SettingsProfileConfig_" + std::to_string(profileIndex);
	}

	//const std::string& SettingsProfileConfig::getName() const
	//{
	//	return ConfigBase::getName();
	//}

	void SettingsProfileConfig::setProfileIndex(size_t newIndex)
	{
		profileIndex = newIndex;
		fileName = getIndexedName();
	}

	void SettingsProfileConfig::requestSave()
	{
		save();
	}

}

