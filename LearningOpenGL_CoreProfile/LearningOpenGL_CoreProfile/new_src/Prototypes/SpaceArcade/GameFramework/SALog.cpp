#include "SALog.h"
#include <iostream>

namespace SA
{
	void log(const char* msg, const char* LogName /*= "Game"*/, LogLevel level /*= LogLevel::LOG*/)
	{
		std::ostream& output = (level == LogLevel::LOG_WARNING || level == LogLevel::LOG_ERROR) ? std::cerr : std::cout;
		output << msg << std::endl;
	}
}

