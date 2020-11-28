#pragma once
#include <string>
#include <unordered_map>

#include "../Tools/DataStructures/MultiDelegate.h"
#include "SASystemBase.h"

namespace SA
{
	using CheatDelegate = MultiDelegate<const std::vector<std::string>&>;

	class CheatSystemBase : public SystemBase
	{
	public:
		bool parseCheat(const std::string& cheatString);
		void getAllCheats(std::vector<std::string>& cheatStrings) const;
		size_t getNumCheats();
	protected:
		//having cheats forward to delegates allows you to parse string into types before broadcasting to subscribers
		void registerCheat(std::string cheat, sp<CheatDelegate> cheatDelegate);
	private:
		std::unordered_map<
			std::string,
			sp<CheatDelegate>
		> cheatStrToDelegate;
	};
}