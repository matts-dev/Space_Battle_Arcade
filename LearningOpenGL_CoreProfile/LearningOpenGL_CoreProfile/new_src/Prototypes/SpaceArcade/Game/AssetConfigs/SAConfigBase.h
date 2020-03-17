#pragma once
#include <string>
#include <functional>
#include "../../../../../Libraries/nlohmann/json.hpp"
#include "../../GameFramework/SAGameEntity.h"

namespace SA
{
	class ConfigBase : public GameEntity
	{
	public:
		using json = nlohmann::json;

	public:
		/* @param configFactor constructs a child class of ConfigBase */
		static sp<ConfigBase> load(std::string filePath, const std::function<sp<ConfigBase>()>& configFactory);
		virtual std::string getRepresentativeFilePath() = 0;

		std::string serialize();
		void deserialize(const std::string& fileAsStr);

		const std::string& getName() const { return name; }
		bool isDeletable() const { return bIsDeletable; }
		const std::string& getOwningModDir() const { return owningModDir; }
	protected:
		void save(); //access restricted, only allow certain classes to save this.

		virtual void onSerialize(json& outData) = 0;
		virtual void onDeserialize(const json& inData) = 0;

	protected: //serializedProperties
		std::string name;
		bool bIsDeletable = true;

	protected: //non serialized properties
		std::string owningModDir;      //do not serialize this, spawn configs should be copy-and-pastable to other mods; set on loading

	};
}