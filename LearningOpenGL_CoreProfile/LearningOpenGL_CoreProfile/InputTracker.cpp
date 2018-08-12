#include "InputTracker.h"



InputTracker::InputTracker()
{
}


InputTracker::~InputTracker()
{
}

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

	std::stack<int> keysToRemove;

	//find all keys that are no longer pressed but still in the container
	for (const auto& key : keysCurrentlyPressed)
	{
		if (glfwGetKey(window, key) != GLFW_PRESS)
		{
			//avoid removing from a container we're iterating over
			keysToRemove.push(key);
		}
	}

	//remove all the keys that were not pressed
	while (!keysToRemove.empty())
	{
		int removeKey = keysToRemove.top();
		keysCurrentlyPressed.erase(removeKey);
		keysToRemove.pop();
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
