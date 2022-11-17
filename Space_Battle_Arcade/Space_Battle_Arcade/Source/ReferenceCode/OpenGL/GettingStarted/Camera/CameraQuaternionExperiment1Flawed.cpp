#include "CameraQuaternionExperiment1Flawed.h"
#include <iostream>
#include <gtx\quaternion.hpp>

//local to .cpp 
namespace
{
	//glfw callbacks are a construct and making registering a bound obj to a member function hard.
	//I'ev opted to just assum there's only every one glfw callback handler that is set statically
	//its memory is cleaned up in camera dtors and can only be set via this .cpp.
	//I suppose a more elegant solution would be to have a static map that allows callbacks for
	//different windows, but I'll implement that if needed. The idea would be you use the window pointer
	//to look into the map and find the appropriate camera handler.
	CameraQuaternionExperiment1Flawed* glfwCallbackHandler = nullptr;

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

void CameraQuaternionExperiment1Flawed::exclusiveGLFWCallbackRegister(GLFWwindow* window)
{
	if (window)
	{
		glfwCallbackHandler = this;
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetCursorEnterCallback(window, &mouseLeftCallback);
		glfwSetScrollCallback(window, &scrollCallback);
	}
	else {
		std::cerr << "warning: null window passed when attempting to register for glfw mouse callbacks" << std::endl;
	}
}


CameraQuaternionExperiment1Flawed::CameraQuaternionExperiment1Flawed(float inFOV)
	: FOV(inFOV)
{
	u_axis = DEFAULT_AXIS_U;
	v_axis = DEFAULT_AXIS_V;
	w_axis = DEFAULT_AXIS_W;
}

CameraQuaternionExperiment1Flawed::~CameraQuaternionExperiment1Flawed()
{
	//clean up what's listening to the glfw window callbacks.
	if (glfwCallbackHandler == this)
	{
		glfwCallbackHandler = nullptr;
	}
}

glm::mat4 CameraQuaternionExperiment1Flawed::getView()
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
void CameraQuaternionExperiment1Flawed::mouseMovedBroken(double xpos, double ypos)
{
	///!-------------- try doing small circles around an imaginary reticle to see why this method doesn't work ------------0
	///!-------------- it will cause a roll to occur over time; other than that it seems to work.
	///!-------------- leaving this in the file as a note on attempting to build a quaternion based camera in such a manner
	using namespace glm;
	if (refocused)
	{
		lastX = xpos;
		lastY = ypos;
		refocused = false;
		return; //don't allow 0s to propagate in equations
	}

	//set up mouse deltas
	float yawFactor = static_cast<float>(lastX - xpos);
	float pitchFactor = static_cast<float>(lastY - ypos);
	yawFactor *= mouseSensitivity;
	pitchFactor *= mouseSensitivity;


	quat yawQ;
	yawQ = angleAxis(yawFactor, v_axis);
	quaternion = yawQ * quaternion;
	updateAxes();

	quat pitchQ;
	pitchQ = angleAxis(pitchFactor, u_axis);
	quaternion = pitchQ * quaternion;
	updateAxes();

	lastX = xpos;
	lastY = ypos;
}


//plane vector attempt (I think I should still investigate this approach, has bugs currently
void CameraQuaternionExperiment1Flawed::mouseMoved(double xpos, double ypos)
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
	float yawFactor = static_cast<float>(lastX - xpos); //these seem backwards
	float pitchFactor = static_cast<float>(ypos - lastY);
	//yawFactor *= mouseSensitivity;
	//pitchFactor *= mouseSensitivity;

	//create an update vector in terms of u and v (yaw and pitch respectively)
	//the uv plane is the plane of the monitor screen.
	vec3 uvPlaneVec = u_axis * yawFactor;
	uvPlaneVec += v_axis * pitchFactor;
	float rotationMagnitude = length(uvPlaneVec);
	uvPlaneVec = normalize(uvPlaneVec);

	//get a rotation axis between the vector in the screen and the direction of the camera.
	vec3 rotationAxis = normalize(cross(uvPlaneVec, -w_axis));

	//rotate the w_axis
	//quat q1(0, 0, 0, 1);
	quat q1;
	q1 = angleAxis(mouseSensitivity * rotationMagnitude, rotationAxis);

	//update the quaternion transform
	quaternion = q1 * quaternion;

	//update all axes
	updateAxes();

	//std::cout << "u:" << u_axis.x << " " << u_axis.y << " " << u_axis.z << " ";
	//std::cout << "v:" << v_axis.x << " " << v_axis.y << " " << v_axis.z << " ";
	//std::cout << "w:" << w_axis.x << " " << w_axis.y << " " << w_axis.z << " " << std::endl;

	lastX = xpos;
	lastY = ypos;

}

void CameraQuaternionExperiment1Flawed::windowFocusedChanged(int focusEntered)
{
	refocused = focusEntered;
}

void CameraQuaternionExperiment1Flawed::mouseWheelUpdate(double xOffset, double yOffset)
{
	if (FOV >= 1.0f && FOV <= 45.0f)
		FOV -= static_cast<float>(yOffset);
	if (FOV <= 1.0f)
		FOV = 1.0f;
	if (FOV >= 45.0)
		FOV = 45.0f;
}

void CameraQuaternionExperiment1Flawed::handleInput(GLFWwindow* window, float deltaTime)
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
		//roll += deltaTime * rollSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		//roll -= deltaTime * rollSpeed;
	}
}

void CameraQuaternionExperiment1Flawed::setPosition(float x, float y, float z)
{
	cameraPosition.x = x;
	cameraPosition.y = y;
	cameraPosition.z = z;
}

void CameraQuaternionExperiment1Flawed::updateAxes()
{
	using namespace glm;

	mat4 transform = toMat4(quaternion);

	u_axis = vec3(transform * vec4(DEFAULT_AXIS_U, 0));
	v_axis = vec3(transform * vec4(DEFAULT_AXIS_V, 0));
	w_axis = vec3(transform * vec4(DEFAULT_AXIS_W, 0));

}
