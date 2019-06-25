#include "SAModSystem.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include "..\GameFramework\SALog.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\..\..\..\Libraries\nlohmann\json.hpp"
#include <sstream>

using json = nlohmann::json;

namespace SA
{

	std::string Mod::getModName()
	{
		return modName;
	}

	std::string Mod::getModDirectoryPath()
	{
		return MODS_DIRECTORY + getModName();
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

				//this mod is still present in filestyem, remove it from set of deleted mods
				deletedMods.erase(MOD_NAME_FROM_DIR);

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
						}
						else
						{
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
			//TODO broadcast mod changing
			activeMod = newMod;
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
		const std::string modDirectoryPath = MODS_DIRECTORY + std::string("/") + modName;
		const std::string modFilePath = modDirectoryPath + std::string("/") + modName + std::string(".json");

		std::error_code fileCheckEC;
		bool bFileAlreadyExists = std::filesystem::exists(modFilePath, fileCheckEC);
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
		std::filesystem::create_directories(modDirectoryPath, mkModFolderEC);
		if (mkModFolderEC)
		{
			log("ModSystem", LogLevel::LOG_ERROR, "Failed to create new mod folder");
			return false;
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
			std::ofstream outFile_RAII(modFilePath /*do not trunc; never overwrite prexisting mods here*/);
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
		
		/*
			//read game persistent storage to figure out default mod
		
		*/


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
}
