
#include "GameFramework/CheatSystemBase.h"
#include <sstream>
#include <vector>
#include <cctype>
#include <algorithm>
#include "Tools/DataStructures/MultiDelegate.h"
#include "GameFramework/SALog.h"

namespace SA
{

	void CheatSystemBase::registerCheat(std::string cheat, sp<CheatDelegate> cheatDelegate)
	{
		//always force cheats to be lowercase
		std::transform(cheat.begin(), cheat.end(), cheat.begin(), [](unsigned char c) {return std::tolower(c); });

		if (cheatStrToDelegate.find(cheat) == cheatStrToDelegate.end())
		{
			cheatStrToDelegate[cheat] = cheatDelegate;
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "detected duplicate cheat being registered");
		}
	}

	bool CheatSystemBase::parseCheat(const std::string& cheatString)
	{
		bool bFoundCheat = false;

		std::string cheatStringLowered = cheatString;
		std::transform(cheatStringLowered.begin(), cheatStringLowered.end(), cheatStringLowered.begin(),
			[](unsigned char c) {return std::tolower(c); });

		std::stringstream ss(cheatStringLowered);
		std::vector<std::string> parsedWords;

		while (ss.good())
		{
			parsedWords.push_back("");
			ss >> parsedWords.back();
		}

		if (parsedWords.size() >= 1)
		{
			auto kvFindIter = cheatStrToDelegate.find(parsedWords[0]);
			if (kvFindIter != cheatStrToDelegate.end())
			{
				bFoundCheat = true;
				if (const auto& delegate = kvFindIter->second)
				{
					delegate->broadcast(parsedWords); 
				}
			}
		}

		return bFoundCheat;
	}

	void CheatSystemBase::getAllCheats(std::vector<std::string>& cheatStrings) const
	{
		//this is a really slow method, should probably do once and cache. Iterating hash map is not like iterating bst
		for (auto& kv_pair : cheatStrToDelegate)
		{
			cheatStrings.push_back(kv_pair.first);
		}
	}

	size_t CheatSystemBase::getNumCheats()
	{
		return cheatStrToDelegate.size();
	}

}

