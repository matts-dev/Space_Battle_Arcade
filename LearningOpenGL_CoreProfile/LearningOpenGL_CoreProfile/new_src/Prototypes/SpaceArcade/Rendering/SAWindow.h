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
		MultiDelegate<double /*xOffset*/, double /*yOffset*/> scrollChanged;
		MultiDelegate<int /*newWidth*/, int /*newHeight*/> framebufferSizeChanged;

		/* mods: GLFW_MOD_SHIFT, GLFW_MOD_CONTROL,GLFW_MOD_ALT, GLFW_MOD_SUPER
		   actions: GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT (kb only) */
		MultiDelegate<int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/> onKeyInput;
		MultiDelegate<int /*button*/, int /*action*/, int /*mods*/> onMouseButtonInput;
		

	//window instances
	public:
		Window(uint32_t width, uint32_t height);
		virtual void postConstruct() override;
		~Window();

		inline GLFWwindow* get() { return window; }
		void markWindowForClose(bool bClose);
		bool shouldClose();
		float getAspect();

		void setViewportToWindowSize();

	private:
		void handleFramebufferSizeChanged(int width, int height);

	private:
		GLFWwindow* window;
	};

}