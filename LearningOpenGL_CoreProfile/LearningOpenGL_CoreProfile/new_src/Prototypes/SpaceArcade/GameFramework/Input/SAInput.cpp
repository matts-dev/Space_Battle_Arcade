#include "SAInput.h"
#include "..\SAGameBase.h"
#include "..\SAWindowSystem.h"

namespace SA
{
	bool InputTracker::isKeyJustPressed(GLFWwindow* window, int key)
	{
		if (!window)
		{
			return false;
		}

		if (glfwGetKey(window, key) == GLFW_PRESS)
		{
			if (keysCurrentlyPressed.find(key) == keysCurrentlyPressed.end())
			{
				keysCurrentlyPressed.insert(key);
				return true;
			}
		}

		return false;
	}

	void InputTracker::updateState(GLFWwindow* window)
	{
		if (!window)
		{
			return;
		}

		//#TODO perhaps use reserved vector for optimized allocations?
		std::stack<int> keysToRemove;
		std::stack<int> buttonsToRemove;

		//find all keys that are no longer pressed but still in the container
		for (const auto& key : keysCurrentlyPressed)
		{
			if (glfwGetKey(window, key) != GLFW_PRESS)
			{
				//avoid removing from a container we're iterating over
				keysToRemove.push(key);
			}
		}
		for (const auto& button : mouseButtonsCurrentlyPressed)
		{
			if (glfwGetMouseButton(window, button) != GLFW_PRESS)
			{
				//avoid removing from a container we're iterating over
				buttonsToRemove.push(button);
			}
		}

		//remove all the keys that were not pressed
		while (!keysToRemove.empty())
		{
			int removeKey = keysToRemove.top();
			keysCurrentlyPressed.erase(removeKey);
			keysToRemove.pop();
		}
		while (!buttonsToRemove.empty())
		{
			int removeButton = buttonsToRemove.top();
			mouseButtonsCurrentlyPressed.erase(removeButton);
			buttonsToRemove.pop();
		}
	}

	bool InputTracker::isKeyDown(GLFWwindow* window, int key)
	{
		if (!window)
		{
			return false;
		}

		if (glfwGetKey(window, key) == GLFW_PRESS)
		{
			//don't insert into pressed keys
			return true;
		}

		return false;
	}

	bool InputTracker::isMouseButtonJustPressed(GLFWwindow* window, int button)
	{
		if (!window)
		{
			return false;
		}

		if (glfwGetMouseButton(window, button) == GLFW_PRESS)
		{
			if (mouseButtonsCurrentlyPressed.find(button) == mouseButtonsCurrentlyPressed.end())
			{
				mouseButtonsCurrentlyPressed.insert(button);
				return true;
			}
		}

		return false;
	}

	bool InputTracker::isMouseButtonDown(GLFWwindow* window, int button)
	{
		if (!window)
		{
			return false;
		}

		if (glfwGetMouseButton(window, button) == GLFW_PRESS)
		{
			//don't insert into pressed keys;
			return true;
		}

		return false;
	}

	SA::MultiDelegate<int /*state*/, int /*modifier_keys*/, int /*scancode*/>& InputProcessor::getKeyEvent(const uint32_t key)
	{
		if (key <= GLFW_KEY_LAST)
		{
			//adjust index to be 0 based by subtracting off first
			return keyEvents[key - SA_FIRST_KEY_CODE];
		}
		else
		{ 
			throw std::runtime_error("invalid key value passed to input processor");
		}
	}

	SA::MultiDelegate<int /*state*/, int /*modifier_keys*/>& InputProcessor::getMouseButtonEvent(const uint32_t button)
	{
		if (button <= GLFW_MOUSE_BUTTON_LAST)
		{
			//adjust index to be 0 based by subtracting off first
			return mouseButtonEvents[button - SA_FIRST_MOUSE_BUTTON];
		}
		else
		{
			throw std::runtime_error("invalid mouse button passed to input processor");
		}
	}

	void InputProcessor::postConstruct()
	{
		GameBase& game = GameBase::get();
		WindowSystem& windowSystem = game.getWindowSystem();

		windowSystem.onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &InputProcessor::handlePrimaryWindowChanging);

		//if window was created before we register, we need to bind to it; just call the handler immediately
		const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow();
		if (primaryWindow)
		{
			handlePrimaryWindowChanging(nullptr, primaryWindow);
		}

	}

	void InputProcessor::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		if (new_window)
		{
			//if we've already bound once to this window, then we're bound to all delegates
			if (!new_window->onKeyInput.hasBoundStrong(*this))
			{
				new_window->onKeyInput.addStrongObj(sp_this(), &InputProcessor::handleKeyInput);
				new_window->onMouseButtonInput.addStrongObj(sp_this(), &InputProcessor::handleMouseButtonInput);
			}
		}
	}

	void InputProcessor::handleKeyInput(int key, int scancode, int action, int mods)
	{
		if (key >= 0 && key <= GLFW_KEY_LAST)
		{

			//putting scancode last intentionally, as this is probably the least significant parameter
			keyEvents[key - SA_FIRST_KEY_CODE].broadcast(action, mods, scancode);
			onKey.broadcast(key, action, mods, scancode);
		}
		else
		{
			//unknown key
		}
	}

	void InputProcessor::handleMouseButtonInput(int button, int action, int mods)
	{
		if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST)
		{
			mouseButtonEvents[button - SA_FIRST_MOUSE_BUTTON].broadcast(action, mods);
			onButton.broadcast(button, action, mods);
		}
		else
		{
			//unknown button
		}
	}

}