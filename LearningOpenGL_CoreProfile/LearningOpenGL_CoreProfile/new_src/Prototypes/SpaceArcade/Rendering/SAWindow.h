#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<cstdint>
#include <unordered_map>
#include "../GameFramework/SAGameEntity.h"
#include "../Tools/DataStructures/MultiDelegate.h"

namespace SA
{
	/*
		A lightweight wrapper for GLFWwindow.

		Window Invariants Requirements:
			-window creation is done on a single thread;		static reference counting for startup/shutdown is not mutex guarded
	*/
	class Window : public GameEntity
	{
	//GLFW management
	private:
		static void startUp();
		static void tryShutDown();

	public:
		MultiDelegate<double /*x*/, double /*y*/> cursorPosEvent;
		MultiDelegate<int /*Enter*/> mouseLeftEvent;
		MultiDelegate<double /*xOffset*/, double /*yOffset*/> scrollCallback;
		

	//window instances
	public:
		Window(uint32_t width, uint32_t height);
		~Window();

		inline GLFWwindow* get() { return window; }
		void markWindowForClose(bool bClose);
		bool shouldClose();
		float getAspect();

	private:
		void setViewportToWindowSize();

	private:
		GLFWwindow* window;
	};

}