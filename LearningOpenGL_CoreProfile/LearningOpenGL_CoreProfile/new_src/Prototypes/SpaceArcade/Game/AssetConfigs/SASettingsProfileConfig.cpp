#include "SASettingsProfileConfig.h"

namespace SA
{



	void SettingsProfileConfig::onSerialize(json& outData)
	{
		json settingsData =
		{
			{"bEnableDevConsole", bEnableDevConsole},
			{"masterVolume", masterVolume}
		};

		std::string indexName = getIndexedName();
		outData.push_back({ fileName, settingsData});
	}

	void SettingsProfileConfig::onDeserialize(const json& inData)
	{
		std::string indexedName = getIndexedName();

		if (!inData.is_null() && inData.contains(indexedName))
		{
			const json& settingProfileData = inData[indexedName];
			if (!settingProfileData.is_null())
			{
				if (settingProfileData.contains("bEnableDevConsole") && !settingProfileData["bEnableDevConsole"].is_null() && settingProfileData["bEnableDevConsole"].is_boolean())
				{
					bEnableDevConsole = settingProfileData["bEnableDevConsole"];
				}
				if (settingProfileData.contains("masterVolume") && !settingProfileData["masterVolume"].is_null() && settingProfileData["masterVolume"].is_number_float())
				{
					masterVolume = settingProfileData["masterVolume"];
				}
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

