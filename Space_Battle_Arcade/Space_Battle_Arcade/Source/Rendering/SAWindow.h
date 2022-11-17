#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include<cstdint>
#include <utility>
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

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// These are provided for API's that bindings to the raw-callbacks to glfw window; since we use that they cannot register a callback
		// So, we forward the callbacks to those API's by directly calling their callbacks.
		// WARNING: Do not use this callbacks! prefer "onKeyInput" unless you're dealing with a c api that requires this be registered to glfw.
		MultiDelegate<GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/> onRawGLFWKeyCallback;
		MultiDelegate<GLFWwindow* /*window*/, unsigned int /*c*/> onRawGLFWCharCallback;
		MultiDelegate<GLFWwindow* /*window*/, double /*xoffset*/, double /*yoffset*/> onRawGLFWScrollCallback;
		MultiDelegate<GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/> onRawGLFWMouseButtonCallback;
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		

	//window instances
	public:
		Window(uint32_t width, uint32_t height);
		virtual void postConstruct() override;
		~Window();

		inline GLFWwindow* get() { return window; }
		void markWindowForClose(bool bClose);
		bool shouldClose();
		float getAspect();
		std::pair<int, int> getFramebufferSize();

		void setViewportToWindowSize();

	private:
		void handleFramebufferSizeChanged(int width, int height);

	private:
		GLFWwindow* window;
	};

}