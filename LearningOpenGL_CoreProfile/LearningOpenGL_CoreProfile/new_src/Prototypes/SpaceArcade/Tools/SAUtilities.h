#pragma once

#include<iostream>
#include<string>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>


namespace SA
{
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
		// Math Utils
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline bool float_equals(float value, float compareValue, float epsilon = 0.001f)
		{
			//equal if value is under epsilon range
			float delta = std::abs(value - compareValue);
			return delta < epsilon;
		}

		glm::vec3 getDifferentVector(glm::vec3 vec);
	}
}