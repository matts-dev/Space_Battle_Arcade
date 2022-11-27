#include "CameraQuaternionFPS.h"
#include <iostream>
#include <glm/gtx/quaternion.hpp>

//local to .cpp 
namespace
{
	//glfw callbacks are a construct and making registering a bound obj to a member function hard.
	//I'ev opted to just assum there's only every one glfw callback handler that is set statically
	//its memory is cleaned up in camera dtors and can only be set via this .cpp.
	//I suppose a more elegant solution would be to have a static map that allows callbacks for
	//different windows, but I'll implement that if needed. The idea would be you use the window pointer
	//to look into the map and find the appropriate camera handler.
	CameraQuaternionFPS* glfwCallbackHandler = nullptr;

	void mouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (glfwCallbackHandler)
		{
			glfwCallbackHandler->mouseMoved(xpos, ypos);
		}
	}

	void mouseLeftCallback(GLFWwindow* window, int enter)
	{
		if (glfwCallbackHandler)
		{
			glfwCallbackHandler->windowFocusedChanged(enter);
		}
	}

	void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		if (glfwCallbackHandler)
		{
			glfwCallbackHandler->mouseWheelUpdate(xOffset, yOffset);
		}
	}
}

void CameraQuaternionFPS::exclusiveGLFWCallbackRegister(GLFWwindow* window)
{
	if (window)
	{
		glfwCallbackHandler = this;
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetCursorEnterCallback(window, &mouseLeftCallback);
		glfwSetScrollCallback(window, &scrollCallback);
	}
	else
	{
		std::cerr << "warning: null window passed when attempting to register for glfw mouse callbacks" << std::endl;
	}
}


CameraQuaternionFPS::CameraQuaternionFPS(float inFOV)
	: FOV(inFOV)
{
	u_axis = DEFAULT_AXIS_U;
	v_axis = DEFAULT_AXIS_V;
	w_axis = DEFAULT_AXIS_W;
}

CameraQuaternionFPS::~CameraQuaternionFPS()
{
	//clean up what's listening to the glfw window callbacks.
	if (glfwCallbackHandler == this)
	{
		glfwCallbackHandler = nullptr;
	}
}

glm::mat4 CameraQuaternionFPS::getView()
{
	//this creates a view matrix from two matrices.
	//the first matrix (ie the matrix on the right which gets applied first) is just translating points by the opposite of the camera's position.
	//the second matrix (ie matrix on the left) is a matrix where the first 3 rows are the U, V, and W axes of the camera.
	//		The matrix with the axes of the camera is a change of basis matrix.
	//		If you think about it, what it does to a vector is a dot product with each of the camera's axes.
	//		Since the camera axes are all normalized, what this is really doing is projecting the vector on the 3 axes of the camera!
	//			(recall that a vector dotted with a normal vector is equivalent of getting the scalar projection onto the normal)
	//		So, the result of the projection is the vector's position in terms of the camera position.
	//		Since we're saying that the camera's axes align with our NDC axes, we get coordinates in terms x y z that are ready for clipping and projection to NDC 
	//So, ultimately the lookAt matrix shifts points based on camera's position, then projects them onto the camera's axes -- which are aligned with OpenGL's axes 

	//glm::vec3 cameraAxisW = glm::normalize(cameraPosition - (w_axis + cameraPosition)); //points towards camera; camera looks down its -z axis (ie down its -w axis)
	//make each row in the matrix a camera's basis vector; glm is column-major
	glm::mat4 cameraBasisProjection;
	cameraBasisProjection[0][0] = u_axis.x;
	cameraBasisProjection[1][0] = u_axis.y;
	cameraBasisProjection[2][0] = u_axis.z;

	cameraBasisProjection[0][1] = v_axis.x;
	cameraBasisProjection[1][1] = v_axis.y;
	cameraBasisProjection[2][1] = v_axis.z;

	cameraBasisProjection[0][2] = w_axis.x;
	cameraBasisProjection[1][2] = w_axis.y;
	cameraBasisProjection[2][2] = w_axis.z;

	glm::mat4 cameraTranslate = glm::translate(glm::mat4(), -1.f * cameraPosition);

	//first translate the camera to origin
	//then project onto camera basis!
	glm::mat4 view = cameraBasisProjection * cameraTranslate;

	return view;
}


//plane vector attempt (I think I should still investigate this approach, has bugs currently
void CameraQuaternionFPS::mouseMoved(double xpos, double ypos)
{
	using namespace glm;
	if (refocused)
	{
		lastX = xpos;
		lastY = ypos;
		refocused = false;
		return; //don't allow 0s to propagate in equations
	}

	//set up mouse deltas
	float xOff = static_cast<float>(xpos - lastX); //these seem backwards
	float yOff = static_cast<float>(lastY - ypos);
	xOff *= mouseSensitivity;
	yOff *= mouseSensitivity;

	pitch += yOff;
	yaw += xOff;

	//update all axes
	updateAxes();


	lastX = xpos;
	lastY = ypos;

	//std::cout << "pitch:" << pitch << " yaw:" << yaw << std::endl;
}

void CameraQuaternionFPS::windowFocusedChanged(int focusEntered)
{
	refocused = focusEntered;
}

void CameraQuaternionFPS::mouseWheelUpdate(double xOffset, double yOffset)
{
	if (FOV >= 1.0f && FOV <= 45.0f)
		FOV -= static_cast<float>(yOffset);
	if (FOV <= 1.0f)
		FOV = 1.0f;
	if (FOV >= 45.0)
		FOV = 45.0f;
}

void CameraQuaternionFPS::handleInput(GLFWwindow* window, float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraPosition += w_axis * cameraSpeed * deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraPosition -= w_axis * cameraSpeed * deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraPosition += u_axis * cameraSpeed * deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraPosition -= u_axis * cameraSpeed * deltaTime;
	}

	float rollSpeed = 20.f;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		roll += deltaTime * rollSpeed;
		updateAxes();
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		roll -= deltaTime * rollSpeed;
		updateAxes();
	}
}

void CameraQuaternionFPS::setPosition(float x, float y, float z)
{
	cameraPosition.x = x;
	cameraPosition.y = y;
	cameraPosition.z = z;
}

void CameraQuaternionFPS::updateAxes()
{
	using namespace glm;

	//identiy quaternion
	quat q1;
	q1 = angleAxis(radians(pitch), DEFAULT_AXIS_U) * q1;
	q1 = angleAxis(radians(-yaw), DEFAULT_AXIS_V) * q1;
	q1 = angleAxis(radians(roll), DEFAULT_AXIS_W) * q1; //this isn't going to work :( 

	quaternion = q1;


	mat4 transform = toMat4(quaternion);
	u_axis = vec3(transform * vec4(DEFAULT_AXIS_U, 0));
	v_axis = vec3(transform * vec4(DEFAULT_AXIS_V, 0));
	w_axis = vec3(transform * vec4(DEFAULT_AXIS_W, 0));
}
