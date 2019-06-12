#pragma once

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <set>
#include <stack>
#include <array>
#include <cstdint>

#include "../SAGameEntity.h"
#include "../../Tools/DataStructures/MultiDelegate.h"

namespace SA
{
	class Window;

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Polling based input
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	class InputTracker final
	{
	public:
		void updateState(GLFWwindow* window);

		bool isKeyJustPressed(GLFWwindow* window, int key);
		bool isKeyDown(GLFWwindow* window, int key);

		bool isMouseButtonJustPressed(GLFWwindow* window, int button);
		bool isMouseButtonDown(GLFWwindow* window, int button);
	private:
		std::set<int> keysCurrentlyPressed;
		std::set<int> mouseButtonsCurrentlyPressed;
	};

#define SA_FIRST_KEY_CODE GLFW_KEY_SPACE
#define SA_FIRST_MOUSE_BUTTON GLFW_MOUSE_BUTTON_1

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Event base input source
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	class InputProcessor : public GameEntity
	{

	public:
		/* invariant: key must have a value <= GLFW_KEY_LAST */
		MultiDelegate<int /*state*/, int /*modifier_keys*/, int /*scancode*/>& getKeyEvent(const uint32_t key);

		/* invariant: button must have value lower <= GLFW_MOUSE_BUTTON_LAST*/
		MultiDelegate<int /*state*/, int /*modifier_keys*/>& getMouseButtonEvent(const uint32_t button);

		/*less performant delegates that broadcast for every key/button event*/
		MultiDelegate<int /*key*/, int /*state*/, int /*modifier_keys*/, int /*scancode*/> onKey;
		MultiDelegate<int /*button*/, int /*state*/, int /*modifier_keys*/> onButton;

	private:
		virtual void postConstruct() override;

	private:
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleKeyInput(int key, int scancode, int action, int mods);
		void handleMouseButtonInput(int button, int action, int mods);

	private:

		/*  Arrays use allow key/buttons to used as indices; eg GLFW_KEY_T is an index (with some value corrections) into these arrays of delegates.
			Note on value corrections: number of keys/buttons is calculated through LAST_KEY - FIRST_LAST
			Note on reordering of scancode and state from original GLFW window source
		*/
		std::array< MultiDelegate<int /*state*/, int /*modifier_keys*/, int /*scancode*/> , GLFW_KEY_LAST - SA_FIRST_KEY_CODE> keyEvents;
		std::array< MultiDelegate<int /*state*/, int /*modifier_keys*/>, GLFW_MOUSE_BUTTON_LAST - SA_FIRST_MOUSE_BUTTON > mouseButtonEvents;

	};
	
}