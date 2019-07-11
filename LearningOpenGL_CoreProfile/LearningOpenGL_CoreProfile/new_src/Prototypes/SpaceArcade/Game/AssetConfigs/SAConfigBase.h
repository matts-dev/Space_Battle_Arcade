#pragma once
#include <string>

namespace SA
{
	class ConfigBase
	{
	public:
		virtual std::string getRepresentativeFilePath() = 0;

	protected:

	protected: //non serialized properties
		std::string owningModDir;      //do not serialize this, spawn configs should be copy-and-pastable to other mods; set on loading

	};
}