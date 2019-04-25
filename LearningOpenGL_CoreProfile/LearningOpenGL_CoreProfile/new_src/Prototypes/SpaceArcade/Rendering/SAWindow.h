#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<cstdint>
#include <unordered_map>

namespace SA
{
	/*
		A lightweight wrapper for GLFWwindow.

		Window Invariants Requirements:
			-window creation is done on a single thread;		static reference counting for startup/shutdown is not mutex guarded
	*/
	class Window
	{
	//GLFW management
	private:
		static void startUp();
		static void shutDown();
		static uint32_t windowInstances;

	//Callback management
	private:
		static std::unordered_map<GLFWwindow*, Window*> windowMap;
		static void _cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		//static void mouseLeftCallback(GLFWwindow* window, int Enter);
		//static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);


		

	//window instances
	public:
		Window(int width, int height);
		~Window();

		inline GLFWwindow* get() { return window; }
		void markWindowForClose(bool bClose);

	private:
		void setViewportToWindowSize();

	private:
		GLFWwindow* window;
	};

}