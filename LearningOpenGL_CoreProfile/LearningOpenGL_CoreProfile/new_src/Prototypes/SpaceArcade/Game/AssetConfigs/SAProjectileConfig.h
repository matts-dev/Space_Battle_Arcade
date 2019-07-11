#pragma once

#include "SAConfigBase.h"


namespace SA
{
	class ProjectileConfig final : public ConfigBase
	{
	public:
		std::string getName() { return name; }
		bool isDeletable() { return bIsDeletable; }

		virtual std::string getRepresentativeFilePath() override;

	private:
		//#todo this is code duplication with spawnConfig, thinking about moving to base class but since these classes
		//entirely manage their serialization, it might be weird for subclasses to have to "remember" to serialize base class members
		//so perhaps a serialization scheme that passes a json to the child classes to append their data. But I don't want to do that
		//right now as I am working on something else and this isn't critical (very little code duplication and this is only the second config so far) 
		std::string name;
		bool bIsDeletable = true;

	};
}