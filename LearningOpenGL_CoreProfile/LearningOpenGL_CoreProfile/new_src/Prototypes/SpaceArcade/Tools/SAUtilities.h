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
		glm::quat getRotationBetween(const glm::vec3& a_n, const glm::vec3& b_n);
		glm::quat degreesVecToQuat(const glm::vec3& rotationInDegrees);

		inline bool anyValueNAN(float a) { return glm::isnan(a); }
		inline bool anyValueNAN(glm::vec3 vec) {return glm::isnan(vec.x) || glm::isnan(vec.y) || glm::isnan(vec.z);}
		inline bool anyValueNAN(glm::vec4 vec) { return glm::isnan(vec.x) || glm::isnan(vec.y) || glm::isnan(vec.z) || glm::isnan(vec.w); };
		inline bool anyValueNAN(glm::quat quat) 
		{ 
			glm::bvec4 vec = glm::isnan(quat);
			return vec.x || vec.y || vec.z || vec.w;
		};


#if _DEBUG | ERROR_CHECK_GL_RELEASE 
#define NAN_BREAK(value)\
if(SA::Utils::anyValueNAN(value))\
{\
	__debugbreak();\
}
#else
#define NAN_BREAK(value) 
#endif //_DEBUG

	}
}