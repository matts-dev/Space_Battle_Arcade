#pragma once

#include<iostream>
#include<string>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include "DataStructures/SATransform.h"
#include <array> 
#include <vector> //perhaps should break things out into "array utils" etc, so we don't have to include these everywhere that wants access to utils
#include <optional>
#include <functional>

namespace SA
{
	class Shader;

	namespace Utils
	{
		/*
		* Attempts to load a text file specified by the first argument into the string specified by the second argument.
		* @param filepath - the path to the text file to load.
		* @param strRef - the string container to which the file's contents will be loaded
		* @return true if file was successfully loaded and move to string, false otherwise
		*/
		bool readFileToString(const std::string filePath, std::string& strRef);

		GLFWwindow* initWindow(const int width, const int height);

		/*
		* Checks if escape is pressed, if so then sets the state of the window to should close.
		* @param An GLFWwindow pointer to close if escape is pressed.
		*/
		void setWindowCloseOnEscape(GLFWwindow* window);

		GLuint loadTextureToOpengl(const char* relative_filepath, int texture_unit = -1, bool useGammaCorrection = false);

		extern const float cubeVerticesWithUVs[36 * 5];

		//unit create cube (that matches the size of the collision cube)
		extern const float unitCubeVertices_Position_Normal[36 * 6];

		extern const float quadVerticesWithUVs[5 * 6];

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Debug Rendering 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void renderDebugWireCube(
			SA::Shader& debugShader,
			const glm::vec3 color,
			const glm::mat4& model,
			const glm::mat4& view,
			const glm::mat4& perspective
		);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Math Utils
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		constexpr float DEG_TO_RADIAN_F = 3.141592f / 180.f;

		inline bool float_equals(float value, float compareValue, float epsilon = 0.001f)
		{
			//equal if value is under epsilon range
			float delta = std::abs(value - compareValue);
			return delta < epsilon;
		}
		inline bool vectorsAreSame(glm::vec3 first, glm::vec3 second, float epsilon = 0.001)
		{
			return float_equals(first.x, second.x, epsilon)
				&& float_equals(first.y, second.y, epsilon)
				&& float_equals(first.z, second.z, epsilon);
		}
		inline bool vectorsAreColinear(glm::vec3 first, glm::vec3 second, float epsilon = 0.001)
		{
			return	vectorsAreSame(first, second, epsilon) || 
					vectorsAreSame(-first, second, epsilon);
		}
		inline glm::vec3 project(glm::vec3 toProject_v, glm::vec3 axisUnitVec_n)
		{
			return glm::dot(toProject_v, axisUnitVec_n) * axisUnitVec_n;
		}

		glm::vec3 getDifferentVector(glm::vec3 vec);
		glm::vec3 getDifferentNonparallelVector(const glm::vec3& vec);
		glm::vec3 getDifferentNonparallelVector_slowwarning(const glm::vec3& vec);
		float getDegreeAngleBetween(const glm::vec3& from_n, const glm::vec3& to_n);
		float getRadianAngleBetween(const glm::vec3& from_n, const glm::vec3& to_n);
		float getCosBetween(const glm::vec3& from_n, const glm::vec3& to_n);
		glm::quat getRotationBetween(const glm::vec3& from_n, const glm::vec3& to_n);
		glm::quat degreesVecToQuat(const glm::vec3& rotationInDegrees);

		inline bool anyValueNAN(float a) { return glm::isnan(a); }
		inline bool anyValueNAN(glm::vec3 vec) {return glm::isnan(vec.x) || glm::isnan(vec.y) || glm::isnan(vec.z);}
		inline bool anyValueNAN(glm::vec4 vec) { return glm::isnan(vec.x) || glm::isnan(vec.y) || glm::isnan(vec.z) || glm::isnan(vec.w); };
		inline bool anyValueNAN(glm::quat quat) 
		{ 
			glm::bvec4 vec = glm::isnan(quat);
			return vec.x || vec.y || vec.z || vec.w;
		};

		size_t hashColor(glm::vec3 color_ldr);
#if _DEBUG | ERROR_CHECK_GL_RELEASE 
#define NAN_BREAK(value)\
if(SA::Utils::anyValueNAN(value))\
{\
	__debugbreak();\
}
#else
#define NAN_BREAK(value) 
#endif //_DEBUG

		glm::vec3 findBoxLow(const std::array<glm::vec4, 8>& localAABB);
		glm::vec3 findBoxMax(const std::array<glm::vec4, 8>& localAABB);

		/** Rough implementation that may have issues*/
		bool rayHitTest_FastAABB(const glm::vec3& boxLow, const glm::vec3& boxMax, const glm::vec3 rayStart, const glm::vec3 rayDir);

		template<typename T>
		inline T square(T val) { return val * val; }


		////////////////////////////////////////////////////////
		// Vector Utils
		////////////////////////////////////////////////////////
		template <typename T>
		bool isValidIndex(const std::vector<T>& arr, size_t idx)
		{
			return arr.size() > 0 && idx < arr.size();
		}

		template <typename T>
		std::optional<size_t> FindValidIndexLoopingFromIndex(
			size_t startIdx, const std::vector<T>& arr, std::function<bool(const T&)> predicate)
		{
			for (size_t i = startIdx; i < arr.size(); ++i)
			{
				if (predicate(arr[i]))
				{
					return i;
				}
			}
			for (size_t i = 0; i < startIdx; ++i) //wrap around
			{
				if (predicate(arr[i]))
				{
					return i;
				}
			}
			return std::nullopt;
		}


		template<typename T>
		void swapAndPopback(std::vector<T>& arr, size_t idx)
		{
			if (isValidIndex(arr, idx))
			{
				if (arr.size() > 1)
				{
					std::iter_swap(arr.begin() + idx, arr.end() - 1);
					arr.pop_back();
				}
				else
				{
					arr.clear(); //most likely faster to just clear it rather than the temp code generated for swap.
				}
			}
		}
	}
}