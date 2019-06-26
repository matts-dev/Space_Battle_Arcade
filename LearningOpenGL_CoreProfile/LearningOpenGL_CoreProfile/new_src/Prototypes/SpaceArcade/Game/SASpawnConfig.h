#pragma once
#include <string>
#include "..\GameFramework\SAGameEntity.h"

namespace SA
{
	class SpawnConfig
	{
		friend class ModelConfigurerEditor_Level;
		class PrivateKey
		{
			friend class Mod;
			PrivateKey() {};
		};

	public:
		const std::string& getName() const { return name; }
		const std::string& getModelFilePath() const { return fullModelFilePath; };
		bool isDeletable() const { return bIsDeleteable; }

		std::string serialize();
		void deserialize(const std::string& str);

		std::string getRepresentativeFilePath();

	public:
		static sp<SpawnConfig> load(std::string filePathCopy);
	private:
		void save(); //access restricted, only allow certain classes to save this.

	private: //serialized properities
		std::string name;
		std::string fullModelFilePath;
		bool bIsDeleteable = true;
		//vector of collision shapes
		//color/material
		//team
	private: //nonserialized properites
		std::string owningModDir;      //do not serialize this, spawn configs should be copy-and-pastable to other mods; set on loading
	};

}
