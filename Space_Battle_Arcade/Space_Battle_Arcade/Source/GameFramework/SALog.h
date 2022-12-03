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

//NOTE: msvc and clang divergence here. 
//MSVC is able to have this work with NO arguments 
//		snprintf(logging::formatBuffer, sizeof(logging::formatBuffer), msg, __VA_ARGS__);\
//But clang compiles with 
//		error: expected expression
//		logf_sa(__FUNCTION__, LogLevel::LOG, "end cleanup al buffers");
//		^
//		Space_Battle_Arcade/Space_Battle_Arcade/Space_Battle_Arcade/Source/GameFramework/SALog.h:24:82: note: expanded from macro 'logf_sa'
//		snprintf(logging::formatBuffer, sizeof(logging::formatBuffer), msg, __VA_ARGS__);\
//																						^
//This post explains you can add ##__VA_ARGS__ to cover the zero argument situation.
//But that appears to be a c++ extension, and maybe not portable. It does seem to work on MSVC though.
//So for now will use it here, but we may need to do some specific compiler checks here to compile out to different versions of the macro.
#define logf_sa(logName, logLevel, msg, ...)\
	{\
		snprintf(logging::formatBuffer, sizeof(logging::formatBuffer), msg, ##__VA_ARGS__);\
		log(logName, logLevel, logging::formatBuffer);\
	}
}