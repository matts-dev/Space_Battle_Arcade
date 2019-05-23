#pragma once

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <set>
#include <stack>

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

