#include "SALog.h"
#include <iostream>

namespace SA
{
	void log(const char* LogName, LogLevel level, const char* msg)
	{
		std::ostream& output = (level == LogLevel::LOG_WARNING || level == LogLevel::LOG_ERROR) ? std::cerr : std::cout;
		output << LogName << " : " << msg << std::endl;
	}
}

