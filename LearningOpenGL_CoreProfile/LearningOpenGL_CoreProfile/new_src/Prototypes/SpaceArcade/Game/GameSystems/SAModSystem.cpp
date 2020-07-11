#include "SAModSystem.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <sstream>

#include "../AssetConfigs/SASpawnConfig.h"
#include "../AssetConfigs/SAProjectileConfig.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../../../../Libraries/nlohmann/json.hpp"
#include "../AssetConfigs/SASettingsProfileConfig.h"
#include "../AssetConfigs/CampaignConfig.h"
#include "../AssetConfigs/SaveGameConfig.h"
#include "../AssetConfigs/DifficultyConfig.h"

using json = nlohmann::json;

namespace SA
{
	std::string getModConfigFilePath()
	{
		return MODS_DIRECTORY + std::string("config.json");
	}

	void Mod::addSpawnConfig(const sp<SpawnConfig>& spawnConfig)
	{
		if (spawnConfigsByName.find(spawnConfig->getName()) == spawnConfigsByName.end())
		{
			std::string configName = spawnConfig->getName();
			sp<SpawnConfig> copySpawnConfig = spawnConfig;

			spawnConfigsByName.insert({ configName, copySpawnConfig });
		}
		else
		{
			log("Mod", LogLevel::LOG_ERROR, "Attempting to add duplicate spawn config");
		}
	}

	void Mod::removeSpawnConfig(sp<SpawnConfig>& spawnConfig)
	{
		std::string name = spawnConfig->getName();
		spawnConfigsByName.erase(name);
	}

	void Mod::deleteSpawnConfig(sp<SpawnConfig>& spawnConfig)
	{
		if (spawnConfig)
		{
			std::string filepath = spawnConfig->getRepresentativeFilePath();

			//validation before deleting a file
			bool bContainsSpawnConfigs = filepath.find("SpawnConfigs") != std::string::npos;
			bool bBeginsWithGameData = filepath.find("GameData") == 0;
			bool bContainsModPath = filepath.find(getModDirectoryPath()) != std::string::npos;
			bool bIsDeletable = spawnConfig->isDeletable();

			if (bContainsSpawnConfigs && bBeginsWithGameData && bContainsModPath)
			{
				removeSpawnConfig(spawnConfig);

				char deleteMsg[1024];
				//will clip files larger than 1024
				snprintf(deleteMsg, 1024, "Deleting file %s", filepath.c_str());
			
				log("Mod", LogLevel::LOG, deleteMsg);

				std::error_code ec;
				std::filesystem::remove(filepath, ec);

				if (ec)
				{
					log("Mod", LogLevel::LOG_ERROR, "failed to remove file");
				}
			}
		}
	}

	void Mod::addProjectileConfig(const sp<ProjectileConfig>& projectileConfig)
	{
		if (projectileConfigsByName.find(projectileConfig->getName()) == projectileConfigsByName.end())
		{
			std::string configName = projectileConfig->getName();
			sp<ProjectileConfig> copyprojectileConfig = projectileConfig;

			projectileConfigsByName.insert({ configName, copyprojectileConfig });
		}
		else
		{
			log("Mod", LogLevel::LOG_ERROR, "Attempting to add duplicate projectile config");
		}
	}

	void Mod::removeProjectileConfig(sp<ProjectileConfig>& projectileConfig)
	{
		std::string name = projectileConfig->getName();
		projectileConfigsByName.erase(name);
	}

	void Mod::addSettingsProfileConfig(const sp<SettingsProfileConfig>& settingsProfileConfig)
	{
		if (settingsProfileConfig)
		{
			settingsProfileConfig->owningModDir = getModDirectoryPath();
			settingsProfileConfig->setProfileIndex(settingsProfiles.size());
			settingsProfiles.push_back(settingsProfileConfig);
		}
	}

	sp<SA::SettingsProfileConfig> Mod::getSettingsProfile(size_t index)
	{
		if (index < NUM_SETTINGS_PROFILES)
		{
			//create enough profiles to get to the target index
			size_t itemOneBased = index + 1;
			while (itemOneBased > settingsProfiles.size())
			{
				addSettingsProfileConfig(new_sp<SettingsProfileConfig>());
			}

			return settingsProfiles[index];
		}

		return nullptr;
	}

	sp<SA::CampaignConfig> Mod::getCampaign(size_t index)
	{
		if (index < MAX_NUM_CAMPAIGNS)
		{
			size_t itemOneBased = index + 1;
			while (itemOneBased > campaigns.size())
			{
				//if we haven't loaded any campaigns, something is probably wrong but return an empty campaign to avoid a crash
				addCampaignConfig(new_sp<CampaignConfig>());
			}
			return campaigns[index];
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "attempting to get a campaign that is out of bounds for max campaigns allowed");
		}

		return nullptr;
	}

	void Mod::addCampaignConfig(const sp<CampaignConfig>& campaignConfig)
	{
		if (campaignConfig)
		{
			campaignConfig->owningModDir = getModDirectoryPath();
			campaignConfig->setCampaignIndex(settingsProfiles.size());
			campaigns.push_back(campaignConfig);
		}
	}

	sp<SA::SaveGameConfig> Mod::getSaveGameConfig() const
	{
		return saveGameData;
	}

	void Mod::addSaveGameConfig(const sp<SaveGameConfig>& inSaveGameConfig)
	{
		saveGameData = inSaveGameConfig;
		saveGameData->owningModDir = getModDirectoryPath();
	}

	void Mod::deleteProjectileConfig(sp<ProjectileConfig>& projectileConfig)
	{
		if (projectileConfig)
		{
			std::string filepath = projectileConfig->getRepresentativeFilePath();

			//validation before deleting a file
			bool bContainsProjectileConfigs = filepath.find("ProjectileConfigs") != std::string::npos;
			bool bBeginsWithGameData = filepath.find("GameData") == 0;
			bool bContainsModPath = filepath.find(getModDirectoryPath()) != std::string::npos;
			bool bIsDeletable = projectileConfig->isDeletable();

			if (bContainsProjectileConfigs && bBeginsWithGameData && bContainsModPath && bIsDeletable)
			{
				removeProjectileConfig(projectileConfig);

				char deleteMsg[1024];
				//will clip files larger than 1024
				snprintf(deleteMsg, 1024, "Deleting file %s", filepath.c_str());

				log("Mod", LogLevel::LOG, deleteMsg);

				std::error_code ec;
				std::filesystem::remove(filepath, ec); 

				if (ec)
				{
					log("Mod", LogLevel::LOG_ERROR, "failed to remove file");
				}
			}
		}
	}

	sp<SA::SpawnConfig> Mod::getDeafultCarrierConfigForTeam(size_t teamIdx)
	{
		if (defaultCarrierSpawnConfigNamesByTeamIdx.size() > 0)
		{
			//wrap around team idx if it is too large
			teamIdx = teamIdx % defaultCarrierSpawnConfigNamesByTeamIdx.size();

			std::string configName = defaultCarrierSpawnConfigNamesByTeamIdx[teamIdx];
	
			auto findIter = spawnConfigsByName.find(configName);
			if (findIter != spawnConfigsByName.end())
			{
				return findIter->second;
			}
		}
		return nullptr;
	}

	void Mod::setDeafultCarrierConfigForTeam(const std::string& configName, size_t teamIdx)
	{
		if (teamIdx < MAX_TEAM_NUM)
		{
			while (defaultCarrierSpawnConfigNamesByTeamIdx.size() < (teamIdx+1))
			{
				defaultCarrierSpawnConfigNamesByTeamIdx.push_back("");
			}

			defaultCarrierSpawnConfigNamesByTeamIdx[teamIdx] = configName;
		}
	}

	size_t Mod::getActiveCampaignIndex() const
	{
		return activeCampaignIdx;
	}

	std::string Mod::getModName()
	{
		return modName;
	}

	std::string Mod::getModDirectoryPath()
	{
		return MODS_DIRECTORY + getModName() + std::string("/");
	}

	std::string Mod::serialize()
	{
		json teamDefaultCarrierData;

		for (const std::string& carrierName : defaultCarrierSpawnConfigNamesByTeamIdx)
		{
			teamDefaultCarrierData.push_back(carrierName);
		}

		json j =
		{
			{"modName", modName},
			{"bIsDeletable", bIsDeletable},
			{"teamDefaultCarriers", teamDefaultCarrierData }
		};

		return j.dump(4);
	}

	void Mod::deserialize(const std::string& jsonStr)
	{
		json j = json::parse(jsonStr);

		//for backwards compatibility, always check if a field is present in the json (ie not null)
		//these checks also make user modifications less likely to cause crashes
		modName = !j["modName"].is_null() ? j["modName"] : "INVALID";
		bIsDeletable = !j["bIsDeletable"].is_null() ? (bool)j["bIsDeletable"] : true;

		if (!j["teamDefaultCarriers"].is_null() && j["teamDefaultCarriers"].is_array())
		{
			const json& teamCarrierData = j["teamDefaultCarriers"];
			defaultCarrierSpawnConfigNamesByTeamIdx.clear();

			for (size_t teamIdx = 0; teamIdx < teamCarrierData.size() && MAX_TEAM_NUM; ++teamIdx)
			{
				defaultCarrierSpawnConfigNamesByTeamIdx.push_back(teamCarrierData[teamIdx]);
			}
		}
	}

	void Mod::writeToFile()
	{
		std::string modJsonStr = serialize();
		{ //RAII scope
			const std::string MOD_JSON_FILE_PATH = getModDirectoryPath() + modName + ".json";

			std::ofstream outFile_RAII(MOD_JSON_FILE_PATH /*do not trunc; never overwrite prexisting mods here*/);
			if (outFile_RAII.is_open())
			{
				outFile_RAII << modJsonStr;
			}
		}
	}

	void Mod::setModName(PrivateKey key, const std::string& newModName)
	{
		modName = newModName;
	}

	void Mod::postConstruct()
	{
		Parent::postConstruct();

		saveGameData = new_sp<SaveGameConfig>(); //provide default save game in the event there isn't one loaded
		difficulty = new_sp<DifficultyConfig>();

		//create all settings profiles before they are serialized/deserialized
		//for (size_t settingsProfileIdx = 0; settingsProfileIdx < NUM_SETTINGS_PROFILES; ++settingsProfileIdx)
		//{
		//	settingsProfiles.push_back(new_sp<SettingsProfileConfig>());
		//	settingsProfiles.back()->profileIndex = settingsProfileIdx;
		//}

	}

	void ModSystem::refreshModList()
	{
		std::map<std::string, sp<Mod>> deletedMods = loadedMods;

		//create directory path
		std::error_code exists_ec;
		if (!std::filesystem::exists(MODS_DIRECTORY, exists_ec))
		{
			std::error_code mkdir_ec;
			std::filesystem::create_directories(MODS_DIRECTORY, mkdir_ec);
			if (!mkdir_ec)
			{
				log("ModSystem", SA::LogLevel::LOG, "Created mod file path");
			}
			else
			{
				log("ModSystem", SA::LogLevel::LOG_ERROR, "Cannot access mod file path! aborting game");
				GameBase::get().startShutdown();
				return;
			}
		}

		if (exists_ec)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to check game file path");
			log("ModSystem", LogLevel::LOG_ERROR, exists_ec.message().c_str());
			return;
		}

		//iterate over folders in mod directory to find valid mods
		for(const std::filesystem::directory_entry& directory_entry : std::filesystem::directory_iterator(MODS_DIRECTORY))
		{
			if (directory_entry.is_directory())
			{
				const std::string MOD_NAME_FROM_DIR = directory_entry.path().filename().string();
				const std::string THIS_MOD_DIR = MODS_DIRECTORY + MOD_NAME_FROM_DIR;
				const std::string MOD_JSON_FILEPATH = THIS_MOD_DIR + std::string("/") + MOD_NAME_FROM_DIR + ".json";

				//this mod is still present in filesystem, remove it from set of deleted mods
				deletedMods.erase(MOD_NAME_FROM_DIR);

				//since this is a "refresh" function, we don't want deserialize a mod we've already loaded;
				//changes to mod should be done on its runtime representation, we don't support changing the json then refreshing file
				if (loadedMods.find(MOD_NAME_FROM_DIR) == loadedMods.end())
				{
					std::ifstream file(MOD_JSON_FILEPATH);
					if (file.is_open())
					{
						std::stringstream mod_data_ss;
						mod_data_ss << file.rdbuf();
						std::string mod_data = mod_data_ss.str();

						sp<Mod> mod = new_sp<Mod>(); 
						mod->deserialize(mod_data);

						std::string modName = mod->getModName();

						bool bModNameMatchesPathName = modName == MOD_NAME_FROM_DIR;
						assert(bModNameMatchesPathName);

						if (bModNameMatchesPathName)
						{
							if(loadedMods.find(modName) == loadedMods.end())
							{
								loadedMods.insert({ modName, mod });
								loadConfigs(mod);
							}
							else
							{
								//notice this checks the mod name the file reported, not the directory's name
								log("ModSystem", SA::LogLevel::LOG_WARNING, "WARNING: ModSystem loaded duplicate mod!");
							}
						}
						else
						{
							char errorMsg[MAX_MOD_NAME_LENGTH + 128];
							//snprintf has safe buffer overflow restrictions on n
							snprintf(errorMsg, MAX_MOD_NAME_LENGTH + 128, "Error, mod name doesn't match file path[%s]; modname[%s]", MOD_NAME_FROM_DIR.c_str(), modName.c_str());
							log("ModSystem", LogLevel::LOG_WARNING, errorMsg);
						}
					}
					else
					{
						log("ModSystem", SA::LogLevel::LOG_WARNING, "WARNING: Failed to read mod json file");
					}
				}
			}
		}
		
		for (const auto& deletedMod : deletedMods)
		{
			loadedMods.erase(deletedMod.first);
		}

		rebuildModArrayView();
	}

	bool ModSystem::setActiveMod(const std::string& modName)
	{
		if (activeMod && activeMod->getModName() == modName)
		{
			//mod already active, early out
			return true;
		}

		const auto& requestModIter = loadedMods.find(modName);
		if (requestModIter != loadedMods.end())
		{
			sp<Mod> newMod = requestModIter->second;

			onActiveModChanging.broadcast(activeMod, newMod);

			activeMod = newMod;

			writeModConfigFile();
			return true;
		}

		return false;
	}

	bool ModSystem::createNewMod(const std::string& modName)
	{
		if (loadedMods.find(modName) != loadedMods.end())
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Attempted to create mod but there already exists a mod with that name");
			return false;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Check File System; Create new mod folder
		/////////////////////////////////////////////////////////////////////////////////////////////
		const std::string MOD_DIR_PATH = MODS_DIRECTORY + std::string("/") + modName;
		const std::string MOD_JSON_FILE_PATH = MOD_DIR_PATH + std::string("/") + modName + std::string(".json");

		std::error_code fileCheckEC;
		bool bFileAlreadyExists = std::filesystem::exists(MOD_JSON_FILE_PATH, fileCheckEC);
		if (fileCheckEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to check if mod.json exists");
			return false;
		}
		if (bFileAlreadyExists)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to check if mod.json exists");
			return false;
		}

		std::error_code mkModFolderEC;
		std::filesystem::create_directories(MOD_DIR_PATH, mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create mod folder");
			return false;
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Models3D"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create models folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Sounds"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create sounds folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Levels"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create levels folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Textures"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create textures folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/SpawnConfigs"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create SpawnConfigs folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/GameSaves"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create game saves folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/ProjectileConfigs"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create projectile configs folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Settings"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create settings configs folder");
		}

		std::filesystem::create_directories(MOD_DIR_PATH + std::string("/Assets/Campaigns/"), mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create campaigns configs folder");
		}
		


		/////////////////////////////////////////////////////////////////////////////////////////////
		// Create new mod object instance
		/////////////////////////////////////////////////////////////////////////////////////////////
		sp<Mod> mod = new_sp<Mod>();
		mod->setModName(Mod::PrivateKey{}, modName);
		//loadedMods.insert({ modName, mod }); //let reading of serialized files populate loadedMods

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Serialize new mod
		/////////////////////////////////////////////////////////////////////////////////////////////
		//std::string modJsonStr = mod->serialize();
		//{ //RAII scope
		//	std::ofstream outFile_RAII(MOD_JSON_FILE_PATH /*do not trunc; never overwrite prexisting mods here*/);
		//	if (outFile_RAII.is_open())
		//	{
		//		outFile_RAII << modJsonStr;
		//	}
		//}
		mod->writeToFile();


		/////////////////////////////////////////////////////////////////////////////////////////////
		// Refresh mod list based on serialization
		/////////////////////////////////////////////////////////////////////////////////////////////

		refreshModList();

		return true;
	}

	bool ModSystem::deleteMod(const std::string& modName)
	{
		//mod must be loaded before we can delete it, 
		const auto& findResult = loadedMods.find(modName);
		if (findResult != loadedMods.end())
		{
			const sp<Mod>& mod = findResult->second;
			if (mod->isDeletable())
			{
				std::string modDir = mod->getModDirectoryPath();
				std::error_code remove_error;
				std::filesystem::remove_all(modDir, remove_error);
				refreshModList();

				if (activeMod && activeMod->getModName() == modName) activeMod = nullptr;

				if (remove_error){ log("ModSystem", LogLevel::LOG_ERROR, "Failed to delete mod"); }
			}
		}
		return false;
	}

	void ModSystem::initSystem()
	{
		//////////////////////////////////////////////////////////////////////////////////////
		//load available mods
		//////////////////////////////////////////////////////////////////////////////////////
		refreshModList();

		std::error_code exists_ec;

		std::string configFilePath = getModConfigFilePath();
		bool bConfigFileExists = std::filesystem::exists(configFilePath, exists_ec);
		if (exists_ec)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to check if mod config.json exists");
		}

		if (bConfigFileExists && !exists_ec)
		{
			std::ifstream inFile(configFilePath);
			if (inFile.is_open())
			{
				std::stringstream ss;
				ss << inFile.rdbuf();
				std::string fileContents = ss.str();
				if (fileContents.length() > 0)
				{
					json configJson = json::parse(fileContents);
					if (!configJson.is_null())
					{
						if (!configJson["start_module"].is_null())
						{
							std::string startModuleName = configJson["start_module"];
							if (!setActiveMod(startModuleName))
							{
								log("ModSystem", LogLevel::LOG_WARNING, "Failed to set start-up module");
							}
						}
					}
				}
			}
		}
	}

	void ModSystem::writeModConfigFile()
	{
		std::error_code exists_ec;
		std::string configFilePath = getModConfigFilePath();

		std::ofstream outFile{ configFilePath, std::ios::trunc };

		if (outFile.is_open())
		{
			json configJson;

			//conditionally write to file; if there's no active module just remove it
			if (activeMod) { configJson["start_module"] = activeMod->getModName(); }

			outFile << configJson;
		}
	}

	void ModSystem::shutdown()
	{
		
	}

	void ModSystem::rebuildModArrayView()
	{
		modArrayView.clear();

		for (auto pairKV : loadedMods)
		{
			modArrayView.push_back(pairKV.second);
		}
	}

	/* 
		@param assetLocation - should be relative to the mod directory; eg "Assets/SpawnConfigs/"
	*/
	template<typename T>
	static void loadConfig(sp<Mod>& mod,
		const std::string& assetLocation,
		void(Mod::*addConfigFunc)(const sp<T>&),
		const std::function<sp<ConfigBase>()>& factoryMethod
	)
	{
		const std::string CONFIG_DIR = mod->getModDirectoryPath() + assetLocation;  

		std::error_code dir_iter_ec;
		for (const std::filesystem::directory_entry& directory_entry : std::filesystem::directory_iterator(CONFIG_DIR, dir_iter_ec))
		{
			const std::filesystem::path& filePathObj = directory_entry.path();
			if (filePathObj.has_extension() && filePathObj.extension().string() == ".json")
			{
				sp<ConfigBase> baseConfig = ConfigBase::load(filePathObj.string(), factoryMethod);

				//while I am trying dynamic casts, this should NOT be happening every tick and is probably okay; 
				//alternatively ConfigBase could be a template, but that requires it to expose a lot to header (filesystem, etc).
				if (sp<T> derivedConfig = std::dynamic_pointer_cast<T>(baseConfig))
				{
					((*mod).*addConfigFunc)(derivedConfig);
				}
				else
				{
					log("ModSystem", LogLevel::LOG_ERROR, "Failed to typecast loaded config");
				}
			}
		}
		if (dir_iter_ec)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create a directory iterator over files in loadConfig.");
		}
	}


	void ModSystem::loadConfigs(sp<Mod>& mod)
	{
		loadConfig<SpawnConfig>(mod, "Assets/SpawnConfigs/", &Mod::addSpawnConfig, [](){ return new_sp<SpawnConfig>(); });
		loadConfig<ProjectileConfig>(mod, "Assets/ProjectileConfigs/", &Mod::addProjectileConfig, []() { return new_sp<ProjectileConfig>(); });
		loadConfig<SettingsProfileConfig>(mod, "Assets/Settings/", &Mod::addSettingsProfileConfig, []() { return new_sp<SettingsProfileConfig>(); });
		loadConfig<CampaignConfig>(mod, "Assets/Campaigns/", &Mod::addCampaignConfig, []() { return new_sp<CampaignConfig>(); });
		loadConfig<SaveGameConfig>(mod, "GameSaves/", &Mod::addSaveGameConfig, []() { return new_sp<SaveGameConfig>(); });
	}

	//void ModSystem::loadSpawnConfigs(sp<Mod>& mod)
	//{
	//	const std::string SPAWN_CONFIG_DIR = mod->getModDirectoryPath() + std::string("Assets/SpawnConfigs/");

	//	std::error_code dir_iter_ec;
	//	for (const std::filesystem::directory_entry& directory_entry : std::filesystem::directory_iterator(SPAWN_CONFIG_DIR, dir_iter_ec))
	//	{
	//		const std::filesystem::path& pathObj = directory_entry.path();
	//		if (pathObj.has_extension() && pathObj.extension().string() == ".json")
	//		{
	//			if (sp<SpawnConfig> spawnConfig = SpawnConfig::load(pathObj.string()))
	//			{
	//				mod->addSpawnConfig(spawnConfig);
	//			}
	//		}
	//	}
	//	if (dir_iter_ec)
	//	{
	//		log("ModSystem", LogLevel::LOG_ERROR, "Failed to create a directory iterator over files.");
	//	}
	//}

}
