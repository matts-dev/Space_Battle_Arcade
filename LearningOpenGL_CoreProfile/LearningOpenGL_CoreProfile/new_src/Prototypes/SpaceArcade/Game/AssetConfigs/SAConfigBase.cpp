#include "SAConfigBase.h"

#include <fstream>
#include <sstream>

#include "../SAModSystem.h"
#include "../SpaceArcade.h"
#include "../../../../../Libraries/nlohmann/json.hpp"

using json = nlohmann::json;


namespace SA
{

	/*static*/ sp<ConfigBase> ConfigBase::load(std::string filePath, const std::function<sp<ConfigBase>()>& configFactory)
	{
		//replace windows filepath separators with unix separators
		for (uint32_t charIdx = 0; charIdx < filePath.size(); ++charIdx)
		{
			if (filePath[charIdx] == '//')
			{
				filePath[charIdx] = '/';
			}
		}

		std::string modPath = "";

		std::string::size_type modCharIdx = filePath.find("mods/");
		if (modCharIdx != std::string::npos && (modCharIdx < filePath.size() - 1))
		{
			std::string::size_type endOfModNameIdx = filePath.find("/", modCharIdx + 5);
			if (endOfModNameIdx != std::string::npos)
			{
				std::string::size_type count = endOfModNameIdx + 1;
				modPath = filePath.substr(0, count);
			}
		}

		//we must have a mod path if we're going to create a config
		//modpath is not serialized with the config so that configs can
		//be copy pasted across mods
		if (modPath.size() > 0)
		{
			std::ifstream inFile(filePath);
			if (inFile.is_open())
			{
				std::stringstream ss;
				ss << inFile.rdbuf();

				std::string fileAsStr = ss.str();
				if (fileAsStr.length() == 0)
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "loading empty config");
				}

				sp<ConfigBase> newConfig = configFactory();
				newConfig->deserialize(fileAsStr);
				newConfig->owningModDir = modPath;

				return newConfig;
			}
		}

		return nullptr;
	}

	std::string ConfigBase::serialize()
	{
		json outData = {
			{"ConfigBase" , {
					{"name", name},
					{"bIsDeletable", bIsDeletable},
				}
			}
		};

		//subclasses should serialize their data under an object with the subclass name
		onSerialize(outData);

		return outData.dump(4);
	}

	void ConfigBase::deserialize(const std::string& fileAsStr)
	{
		const json rootData = json::parse(fileAsStr);
		if (!rootData.is_null() && rootData.contains("ConfigBase"))
		{
			const json& baseData = rootData["ConfigBase"];

			//for backwards compatibility, make sure to contains check all fields
			name = baseData.contains("name") ? baseData["name"] : "";
			bIsDeletable = baseData.contains("bIsDeletable") ? (bool)baseData["bIsDeletable"] : true;

			onDeserialize(rootData);
		}
	}

	void ConfigBase::save() //access restricted, only allow certain classes to save this.
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			std::string filepath = getRepresentativeFilePath();
			std::ofstream outFile(filepath);
			if (outFile.is_open())
			{
				outFile << serialize();
			}
		}
	}


}

