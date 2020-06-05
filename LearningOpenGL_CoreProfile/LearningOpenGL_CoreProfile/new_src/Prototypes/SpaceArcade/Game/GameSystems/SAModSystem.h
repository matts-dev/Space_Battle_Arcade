#pragma once

#include <map>
#include <vector>
#include <string>
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../../GameFramework/SASystemBase.h"

namespace SA
{
	//forward declarations
	class SpawnConfig;
	class ProjectileConfig;

	////////////////////////////////////////////////////////////////////
	// Constants
	////////////////////////////////////////////////////////////////////
	constexpr std::size_t MAX_MOD_NAME_LENGTH = 512;
	constexpr size_t MAX_TEAM_NUM = 5;
	const char* const MODS_DIRECTORY = "GameData/mods/";
	std::string getModConfigFilePath();


	////////////////////////////////////////////////////////////////////
	// Mod Objects
	////////////////////////////////////////////////////////////////////

	class Mod : public GameEntity
	{
		/*#TODO #cleancode create a map from config type to its instances. Either create a template type to be the access that has all the boilerplate code or
			do something like this post to emulate reflection to remove code duplication https://stackoverflow.com/questions/9859390/use-data-type-class-type-as-key-in-a-map
		*/
	public:
		//////////////////////////////////////////////////////////
		// Grants Module System specific private functions 
		// private constructor meals only friends can create key
		//////////////////////////////////////////////////////////
		class PrivateKey
		{
			friend class ModSystem;
		private: 
			PrivateKey() {};
		};

	public:
		////////////////////////////////////////////////////////////////////
		// Mod public api
		////////////////////////////////////////////////////////////////////
		std::string getModName();
		std::string getModDirectoryPath();
		bool isDeletable() { return bIsDeletable; }

		////////////////////////////////////////////////////////
		//SPAWN CONFIGS
		////////////////////////////////////////////////////////
		/** WARNING: Care must be taken not to call functions modify the return obj while iterating */
		const std::map<std::string, sp<SpawnConfig>>& getSpawnConfigs() { return spawnConfigsByName; }
		void addSpawnConfig(sp<SpawnConfig>& spawnConfig);
		void removeSpawnConfig(sp<SpawnConfig>& spawnConfig);

		/** WARNING: this will delete the spawn config from the file system! */
		void deleteSpawnConfig(sp<SpawnConfig>& spawnConfig);

		////////////////////////////////////////////////////////
		//PROJECTILE CONFIGS
		////////////////////////////////////////////////////////
		/** WARNING: Care must be taken not to call functions that modify the return obj while iterating */
		const std::map<std::string, sp<ProjectileConfig>>& getProjectileConfigs() { return projectileConfigsByName; }
		void addProjectileConfig(sp<ProjectileConfig>& projectileConfig);
		void removeProjectileConfig(sp<ProjectileConfig>& projectileConfig);

		/** WARNING: this will delete the Projectile config from the file system! */
		void deleteProjectileConfig(sp<ProjectileConfig>& projectileConfig);

		//#TODO templatize modifying configs so there isn't code duplication for each config type
		sp<SpawnConfig> getDeafultCarrierConfigForTeam(size_t teamIdx);
		void setDeafultCarrierConfigForTeam(const std::string& configName, size_t teamIdx);

		////////////////////////////////////////////////////////////////////
		// Serialization
		////////////////////////////////////////////////////////////////////
		std::string serialize() ;
		void deserialize(const std::string& str);
		void writeToFile();

	public: //locked methods for construction
		void setModName(PrivateKey key, const std::string& newModName);

	private:
		std::string modName;
		bool bIsDeletable = true;

		std::vector<std::string> defaultCarrierSpawnConfigNamesByTeamIdx;
		std::map<std::string, sp<SpawnConfig>> spawnConfigsByName;
		std::map<std::string, sp<ProjectileConfig>> projectileConfigsByName;
	};

	////////////////////////////////////////////////////////////////////
	// MOD SYSTEM
	////////////////////////////////////////////////////////////////////
	class ModSystem : public SystemBase
	{
	public: //events
		MultiDelegate<const sp<Mod>& /*previous*/, const sp<Mod>& /*active*/> onActiveModChanging;

	public:
		void refreshModList();

		/* returns true if requested mod is now the active mod */
		bool setActiveMod(const std::string& modName);
		void writeModConfigFile();
		const sp<Mod>& getActiveMod() { return activeMod; }

		bool createNewMod(const std::string& modName);
		bool deleteMod(const std::string& modName);

		/** Be mindful of adding adding/removing mods while iterating over this array; 
		it is a view of the current mod list, not the actual container. 
		Changes to the mod list will influence the return value that will invalidate any iterators*/
		inline const std::vector<sp<Mod>>& getMods() { return modArrayView; }

	private:
		virtual void initSystem() override;
		virtual void shutdown() override;

		void rebuildModArrayView();

		void loadConfigs(sp<Mod>& mod);
		//void loadSpawnConfigs(sp<Mod>& mod);
	private:
		sp<Mod> activeMod = nullptr;

		//Mods
		std::map<std::string, sp<Mod>> loadedMods;
		std::vector<sp<Mod>> modArrayView;
	};















































}