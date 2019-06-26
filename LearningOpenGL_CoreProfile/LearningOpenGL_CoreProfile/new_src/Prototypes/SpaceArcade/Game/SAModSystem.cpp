#include "SAModSystem.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include "..\GameFramework\SALog.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\..\..\..\Libraries\nlohmann\json.hpp"
#include <sstream>
#include "SASpawnConfig.h"

using json = nlohmann::json;

namespace SA
{
	std::string getModConfigFilePath()
	{
		return MODS_DIRECTORY + std::string("config.json");
	}

	void Mod::addSpawnConfig(sp<SpawnConfig>& spawnConfig)
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
		json j =
		{
			{"modName", modName},
			{"bIsDeletable", bIsDeletable}
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
	}

	void Mod::setModName(PrivateKey key, const std::string& newModName)
	{
		modName = newModName;
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
								loadSpawnConfigs(mod);
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


		/////////////////////////////////////////////////////////////////////////////////////////////
		// Create new mod object instance
		/////////////////////////////////////////////////////////////////////////////////////////////
		sp<Mod> mod = new_sp<Mod>();
		mod->setModName(Mod::PrivateKey{}, modName);
		//loadedMods.insert({ modName, mod }); //let reading of serialized files populate loadedMods

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Serialize new mod
		/////////////////////////////////////////////////////////////////////////////////////////////
		std::string modJsonStr = mod->serialize();
		{ //RAII scope
			std::ofstream outFile_RAII(MOD_JSON_FILE_PATH /*do not trunc; never overwrite prexisting mods here*/);
			if (outFile_RAII.is_open())
			{
				outFile_RAII << modJsonStr;
			}
		}


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


	void ModSystem::loadSpawnConfigs(sp<Mod>& mod)
	{
		const std::string SPAWN_CONFIG_DIR = mod->getModDirectoryPath() + std::string("Assets/SpawnConfigs/");

		std::error_code dir_iter_ec;
		for (const std::filesystem::directory_entry& directory_entry : std::filesystem::directory_iterator(SPAWN_CONFIG_DIR, dir_iter_ec))
		{
			const std::filesystem::path& pathObj = directory_entry.path();
			if (pathObj.has_extension() && pathObj.extension().string() == ".json")
			{
				if (sp<SpawnConfig> spawnConfig = SpawnConfig::load(pathObj.string()))
				{
					mod->addSpawnConfig(spawnConfig);
				}
			}
		}
		if (dir_iter_ec)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create a directory iterator over spawn configs");
		}
	}

}
