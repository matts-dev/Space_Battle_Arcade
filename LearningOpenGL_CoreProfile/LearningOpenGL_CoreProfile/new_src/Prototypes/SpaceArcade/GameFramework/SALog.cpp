#include "SALog.h"
#include <iostream>
#include "SAGameBase.h"
#include <string>

namespace SA
{
	void log(const char* logName, LogLevel level, const char* msg)
	{
		static GameBase& game = GameBase::get();
		std::string frame = "[" + std::to_string(game.getFrameNumber()) + "]";

		std::ostream& output = (level == LogLevel::LOG_WARNING || level == LogLevel::LOG_ERROR) ? std::cerr : std::cout;
		output << logName << " " << frame << " : " << msg << std::endl;
	}
}

