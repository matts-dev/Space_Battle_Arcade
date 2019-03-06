#pragma once

struct GLFWwindow;

class ISATDemo
{
public:
	ISATDemo(int width, int height) {}
	virtual void tickGameLoop(GLFWwindow* window) = 0;
	virtual void handleModuleFocused(GLFWwindow* window) = 0;
};