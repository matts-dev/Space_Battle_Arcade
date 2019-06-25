#pragma once

#include "..\GameFramework\SASystemBase.h"
#include <map>
#include <vector>
#include "..\Tools\DataStructures\MultiDelegate.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////
	// CONSTANTS
	////////////////////////////////////////////////////////////////////
	constexpr std::size_t MAX_MOD_NAME_LENGTH = 512;
	const char* const MODS_DIRECTORY = "GameData/mods/";
	std::string getModConfigFilePath();


	////////////////////////////////////////////////////////////////////
	// Mod Objects
	////////////////////////////////////////////////////////////////////

	class Mod : public GameEntity
	{
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
		std::string getModName();
		bool isDeletable() { return bIsDeletable; }
		std::string getModDirectoryPath();


		//this implementation uses json strings
		std::string serialize() ;
		void deserialize(const std::string& str);

	public: //locked methods for construction
		void setModName(PrivateKey key, const std::string& newModName);

	private:
		std::string modName;
		bool bIsDeletable = true;
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

	private:
		sp<Mod> activeMod = nullptr;
		std::map<std::string, sp<Mod>> loadedMods;
		std::vector<sp<Mod>> modArrayView;
	};


}