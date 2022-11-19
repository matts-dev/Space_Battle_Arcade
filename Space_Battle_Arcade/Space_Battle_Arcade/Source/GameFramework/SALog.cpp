#include "GameFramework/SALog.h"
#include <iostream>
#include "GameFramework/SAGameBase.h"
#include <string>

namespace SA
{
	namespace logging
	{
		char formatBuffer[10240];
	}

	void log(const char* logName, LogLevel level, const char* msg)
	{
		static GameBase& game = GameBase::get();
		std::string frame = "[" + std::to_string(game.getFrameNumber()) + "]";

		std::ostream& output = (level == LogLevel::LOG_WARNING || level == LogLevel::LOG_ERROR) ? std::cerr : std::cout;
		output << logName << " " << frame << " : " << msg << std::endl;
	}






}

