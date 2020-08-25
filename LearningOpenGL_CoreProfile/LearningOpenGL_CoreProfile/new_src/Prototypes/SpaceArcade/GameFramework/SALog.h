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

	/** #TODO Logging system will probably need more fleshing out*/
	void log(const char* logName, LogLevel level, const char* msg);

	namespace logging
	{
		extern char formatBuffer[10240];
	}

#define logf_sa(logName, logLevel, msg, ...)\
	{\
		snprintf(logging::formatBuffer, sizeof(logging::formatBuffer), msg, __VA_ARGS__);\
		log(logName, logLevel, logging::formatBuffer);\
	}
}