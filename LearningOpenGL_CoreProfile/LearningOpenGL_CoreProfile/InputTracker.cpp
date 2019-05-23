#include "InputTracker.h"

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

	//TODO perhaps use reserved vector for optimized allocations?
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

