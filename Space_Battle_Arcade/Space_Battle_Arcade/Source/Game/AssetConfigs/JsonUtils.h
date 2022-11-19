#pragma once

#include "Libraries/nlohmann/json.hpp"
#include "../../Tools/PlatformUtils.h"
#include "GameFramework/SALog.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODOs/REFACTORS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//-always use json as first argument, this will lead to cleaner looking code that aligns when reading from same json obj


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This series of macros make serialization boilerplate down to a single line.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//macro to allow refactoring code to update serialized field in json
//sizeof enforces compiler to check that refactors to field name are still valid. All c++ fields will have a non-zero size
//this may be paying a runtime cost, but hopefully smart compilers can optimize this out, but leaving us with compile safety
#define SYMBOL_TO_STR(field) sizeof(field) > 0 ? #field : #field

#define STOP_DEBUGGER_ON_READ_FAILS 1

/** Code at end of macros that logs read fails */
#if STOP_DEBUGGER_ON_READ_FAILS
#define READ_FAIL_ELSE_CODE_INJECTION(out)\
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
// write macros
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define JSON_WRITE(field, jsonObj)\
jsonObj[SYMBOL_TO_STR(field)] = field

#define JSON_WRITE_OPTIONAL(toptional_field, jsonObj)\
if (toptional_field.has_value())\
{\
	jsonObj[SYMBOL_TO_STR(toptional_field)] = *toptional_field;\
}\
else\
{\
	jsonObj[SYMBOL_TO_STR(toptional_field)] = nullptr;\
}

#define JSON_WRITE_VEC3(field_vec3, jsonObj)\
{\
	/*may be a more efficient way to do this.*/\
	json valueArray;\
	valueArray.push_back(field_vec3.x);\
	valueArray.push_back(field_vec3.y);\
	valueArray.push_back(field_vec3.z);\
	jsonObj[SYMBOL_TO_STR(field_vec3)] = valueArray;\
}

#define JSON_WRITE_OPTIONAL_VEC3(optional_field_vec3, jsonObj)\
if (optional_field_vec3.has_value())\
{\
	json valueArray; \
	valueArray.push_back(optional_field_vec3->x); \
	valueArray.push_back(optional_field_vec3->y); \
	valueArray.push_back(optional_field_vec3->z); \
	jsonObj[SYMBOL_TO_STR(optional_field_vec3)] = valueArray; \
}\
else\
{\
	jsonObj[SYMBOL_TO_STR(optional_field_vec3)] = nullptr;\
}\


#define JSON_WRITE_VEC4(field_vec4, jsonObj)\
{\
	/*may be a more efficient way to do this.*/\
	json valueArray;\
	valueArray.push_back(field_vec4.x);\
	valueArray.push_back(field_vec4.y);\
	valueArray.push_back(field_vec4.z);\
	valueArray.push_back(field_vec4.w);\
	jsonObj[SYMBOL_TO_STR(field_vec4)] = valueArray;\
}

#define JSON_WRITE_QUAT(field_quat, jsonObj)\
JSON_WRITE_VEC4(field_quat, jsonObj) //use same logic as vec4

#define JSON_WRITE_OPTIONAL_VEC4(optional_field_vec4, jsonObj)\
if (optional_field_vec4.has_value())\
{\
	json valueArray; \
	valueArray.push_back(optional_field_vec4->x); \
	valueArray.push_back(optional_field_vec4->y); \
	valueArray.push_back(optional_field_vec4->z); \
	valueArray.push_back(optional_field_vec4->w); \
	jsonObj[SYMBOL_TO_STR(optional_field_vec4)] = valueArray; \
}\
else\
{\
	jsonObj[SYMBOL_TO_STR(optional_field_vec4)] = nullptr;\
}\


#define JSON_WRITE_TRANSFORM(transform, jsonObj)\
{\
	json transformJson;\
	JSON_WRITE_VEC3(transform.position, transformJson);\
	JSON_WRITE_VEC3(transform.scale, transformJson);\
	JSON_WRITE_QUAT(transform.rotQuat, transformJson);\
	jsonObj[SYMBOL_TO_STR(transform)] = transformJson;\
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read macros that will stop debugger (if set above)  on read fails
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// read bool
////////////////////////////////////////////////////////

#define READ_JSON_BOOL_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasBool(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\

/** injects code to read bool or return default valueReads*/
#define READ_JSON_BOOL(out, jsonObj)\
READ_JSON_BOOL_OPTIONAL(out, jsonObj)\
READ_FAIL_ELSE_CODE_INJECTION(out) /*adds else so that it is no longer optional*/

////////////////////////////////////////////////////////
// read float
////////////////////////////////////////////////////////
#define READ_JSON_FLOAT_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasFloat(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}

#define READ_JSON_FLOAT(out, jsonObj)\
READ_JSON_FLOAT_OPTIONAL(out, jsonObj)\
READ_FAIL_ELSE_CODE_INJECTION(out) /*add else so that it is no longer optional*/


////////////////////////////////////////////////////////
// read vec3
////////////////////////////////////////////////////////

#define READ_JSON_VEC3_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasArray(jsonObj, SYMBOL_TO_STR(out)))\
{\
	const json& vec3Array = jsonObj[SYMBOL_TO_STR(out)];\
	if(vec3Array.size() >= 3)\
	{\
		glm::vec3 proxy;\
		proxy.x = vec3Array[0];\
		proxy.y = vec3Array[1];\
		proxy.z = vec3Array[2];\
		out = proxy; /*proxy allows std::optional<vec3> too, as out.x should be out-> for optionals*/\
	}\
	else\
	{\
		/* Useing cstyle string concatenation in log category*/\
		log(__FUNCTION__, LogLevel::LOG_ERROR, SYMBOL_TO_STR(out)":has array, but array does not have correct size");\
	}\
}\

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read vec4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define READ_JSON_VEC4_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasArray(jsonObj, SYMBOL_TO_STR(out)))\
{\
	const json& vec4Array = jsonObj[SYMBOL_TO_STR(out)];\
	if(vec4Array.size() >= 4)\
	{\
		glm::vec4 proxy;\
		proxy.x = vec4Array[0];\
		proxy.y = vec4Array[1];\
		proxy.z = vec4Array[2];\
		proxy.w = vec4Array[3];\
		out = proxy; /*proxy allows std::optional<vec3> too, as out.x should be out-> for optionals*/\
	}\
	else\
	{\
		/* Useing cstyle string concatenation in log category*/\
		log(__FUNCTION__, LogLevel::LOG_ERROR, SYMBOL_TO_STR(out)":has array, but array does not have correct size");\
	}\
}\

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read quat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define READ_JSON_QUAT_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasArray(jsonObj, SYMBOL_TO_STR(out)))\
{\
	const json& vec4Array = jsonObj[SYMBOL_TO_STR(out)];\
	if(vec4Array.size() >= 4)\
	{\
		glm::quat proxy;\
		proxy.x = vec4Array[0];\
		proxy.y = vec4Array[1];\
		proxy.z = vec4Array[2];\
		proxy.w = vec4Array[3];\
		out = proxy; /*proxy allows std::optional<vec3> too, as out.x should be out-> for optionals*/\
	}\
	else\
	{\
		/* Using cstyle string concatenation in log category*/\
		log(__FUNCTION__, LogLevel::LOG_ERROR, SYMBOL_TO_STR(out)":has array, but array does not have correct size");\
	}\
}\


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read transform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define READ_JSON_TRANSFORM_OPTIONAL(transform, jsonObj)\
if (SA::JsonUtils::has(jsonObj, SYMBOL_TO_STR(transform)))\
{\
	const json& transformJson = jsonObj[SYMBOL_TO_STR(transform)];\
	READ_JSON_VEC3_OPTIONAL(transform.position, transformJson);\
	READ_JSON_VEC3_OPTIONAL(transform.scale, transformJson);\
	READ_JSON_QUAT_OPTIONAL(transform.rotQuat, transformJson)\
}

////////////////////////////////////////////////////////
// read string
////////////////////////////////////////////////////////
#define READ_JSON_STRING_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasString(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\

#define READ_JSON_STRING(out, jsonObj)\
READ_JSON_STRING_OPTIONAL(out, jsonObj)\
READ_FAIL_ELSE_CODE_INJECTION(out) /*add else so that it is no longer optional*/


////////////////////////////////////////////////////////
// read integer numbers
//
// hint: using #include<cstind> int64_t etc to prevent intellisense issues.
////////////////////////////////////////////////////////
#define READ_JSON_INT(out, jsonObj)\
if (SA::JsonUtils::hasInt(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\
READ_FAIL_ELSE_CODE_INJECTION(out)

#define READ_JSON_INT_OPTIONAL(out, jsonObj)\
if (SA::JsonUtils::hasInt(jsonObj, SYMBOL_TO_STR(out)))\
{\
	out = jsonObj[SYMBOL_TO_STR(out)];\
}\


////////////////////////////////////////////////////////
// read transform
////////////////////////////////////////////////////////



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

