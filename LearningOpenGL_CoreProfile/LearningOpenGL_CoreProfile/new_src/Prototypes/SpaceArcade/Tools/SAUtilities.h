#pragma once

#include<iostream>
#include<string>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

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

		extern const float cubeVerticesWithUVs[36 * 5];
		extern const float cubeVerticesWithNormals[36 * 6];
	}
}