#pragma once

#include "../../../../../Libraries/nlohmann/json.hpp"
#include "../../Tools/PlatformUtils.h"
#include "../../GameFramework/SALog.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This series of macros make serialization boilerplate down to a single line.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//macro to allow refactoring code to update serialized field in json
//sizeof enforces compiler to check that refactors to field name are still valid. All c++ fields will have a non-zero size
#define SYMBOL_TO_STR(field) sizeof(field) > 0 ? #field : #field

#define STOP_DEBUGGER_ON_READ_FAILS 1

/** Code at end of macros that logs read fails */
#if STOP_DEBUGGER_ON_READ_FAILS
#define READ_FAIL_CODE_INJECTION(out)\
else\
{\
	log(__FUNCTION__, LogLevel::LOG_ERROR, "Failed to read json field " #out);\
	STOP_DEBUGGER_HERE();\
}
#elif
else\
{\
	log(__FUNCTION__, LogLevel::LOG_ERROR, "Failed to read json field " #out); \
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read macros that will stop debugger (if set above)  on read fails
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// read bool
////////////////////////////////////////////////////////

/** injects code to read bool or return default valueReads*/
#define READ_JSON_BOOL(out, jsonObj)\
if (SA::JsonUtils::hasBool(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\
READ_FAIL_CODE_INJECTION(out)

////////////////////////////////////////////////////////
// read float
////////////////////////////////////////////////////////
#define READ_JSON_FLOAT(out, jsonObj)\
if (SA::JsonUtils::hasFloat(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\
READ_FAIL_CODE_INJECTION(out)

////////////////////////////////////////////////////////
// read string
////////////////////////////////////////////////////////
#define READ_JSON_STRING(out, jsonObj)\
if (SA::JsonUtils::hasString(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\
READ_FAIL_CODE_INJECTION(out)

////////////////////////////////////////////////////////
// read integer numbers
////////////////////////////////////////////////////////
#define READ_JSON_INT(out, jsonObj)\
if (SA::JsonUtils::hasInt(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\
READ_FAIL_CODE_INJECTION(out)


namespace SA
{
	class JsonUtils
	{
		using json = nlohmann::json;
	public:
		static inline bool has(const json& jsonObj, const std::string& key)
		{
			return has(jsonObj, key.c_str());
		}
		static inline bool has(const json& jsonObj, const char*const key)
		{
			return jsonObj.contains(key) && !jsonObj[key].is_null();
		}
		static inline bool hasBool(const json& jsonObj, const char*const key)
		{
			return has(jsonObj, key) && jsonObj[key].is_boolean();
		}

		static inline bool hasFloat(const json& jsonObj, const char*const key) 
		{
			return has(jsonObj, key) && jsonObj[key].is_number_float();
		}

		static inline bool hasString(const json& jsonObj, const char*const key)
		{
			return has(jsonObj, key) && jsonObj[key].is_string();
		}

		static inline bool hasInt(const json& jsonObj, const char*const key)
		{
			return has(jsonObj, key) && jsonObj[key].is_number_integer();
		}

		static inline bool hasArray(const json& jsonObj, const char*const key)
		{
			return has(jsonObj, key) && jsonObj[key].is_array();
		}



	};
}

