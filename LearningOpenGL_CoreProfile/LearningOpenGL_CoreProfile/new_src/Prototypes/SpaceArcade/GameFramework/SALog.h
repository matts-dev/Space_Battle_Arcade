#pragma once

#include<cstdint>

namespace SA
{
	enum class LogLevel : uint8_t 
	{
		LOG,
		LOG_WARNING,
		LOG_ERROR
	};

	/** TODO Logging system will probably need more fleshing out*/
	void log(const char* msg, const char* LogName = "Game", LogLevel level = LogLevel::LOG);
}