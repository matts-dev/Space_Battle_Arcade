#include "FreeCamera.h"
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
	FreeCamera* glfwCallbackHandler = nullptr;

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

void FreeCamera::exclusiveGLFWCallbackRegister(GLFWwindow* window)
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


FreeCamera::FreeCamera(float inFOV)
	: FOV(inFOV)
{
	u_axis = DEFAULT_AXIS_U;
	v_axis = DEFAULT_AXIS_V;
	w_axis = DEFAULT_AXIS_W;
}

FreeCamera::~FreeCamera()
{
	//clean up what's listening to the glfw window callbacks.
	if (glfwCallbackHandler == this)
	{
		glfwCallbackHandler = nullptr;
	}
}

glm::mat4 FreeCamera::getView()
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

#define PLANAR_MOVEMENT 1
void FreeCamera::mouseMoved(double xpos, double ypos)
{
	//there's two approaches that I've come up with, the preprocessor definition switches between them.
#if PLANAR_MOVEMENT == 1
	mouseMovedPlanar(xpos, ypos);
#else 
	mouseMovedDiscrete(xpos, ypos);
#endif // PLANAR_MOVEMENT
}

void FreeCamera::windowFocusedChanged(int focusEntered)
{
	refocused = focusEntered;
}

void FreeCamera::mouseWheelUpdate(double xOffset, double yOffset)
{
	if (FOV >= 1.0f && FOV <= 45.0f)
		FOV -= static_cast<float>(yOffset);
	if (FOV <= 1.0f)
		FOV = 1.0f;
	if (FOV >= 45.0)
		FOV = 45.0f;
}

void FreeCamera::handleInput(GLFWwindow* window, float deltaTime)
{
	//DISCLAIMER: make sure delta time passed is not 0!
	float epsilon = 100 * std::numeric_limits<float>::epsilon();
	if (abs(deltaTime) < epsilon)
	{
		return;
	}

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

	float rollSpeed = 100.f;
	float roll = 0.f;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		roll += deltaTime * rollSpeed;
		updateRoll(roll);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		roll -= deltaTime * rollSpeed;
		updateRoll(roll);
	}
}

void FreeCamera::setPosition(float x, float y, float z)
{
	cameraPosition.x = x;
	cameraPosition.y = y;
	cameraPosition.z = z;
}

//helper for float comparisons
bool valuesAreSame(float first, float second)
{
	const float epi = 100 * std::numeric_limits<float>::epsilon();
	return abs(first - second) < epi;
}

//helper for vector comparisons
bool vectorsAreSame(glm::vec3 first, glm::vec3 second)
{
	return valuesAreSame(first.x, second.x) && valuesAreSame(first.y, second.y) && valuesAreSame(first.z, second.z);
}

/**
	Technique: 
		1. Get a vector from camera in current looking direction
		2. Get a vector to the look at position (from the current camera's position
		3. Calculate the angle between the two vectors by using the dot product == |A||B|cos(theta)) (requries both vectors to be normalized)
		4. Calculate the axis of rotation for that angle via cross product (view vector CROSS look at vector)
		5. Calculate a quaternion for such a rotation
		6. Apply quaterion to current rotation quaternion
*/
void FreeCamera::lookAt(glm::vec3 position)
{
	using namespace glm;

	//prestep -- make sure we're not trying to lookat the position we're already looking at
	if (position == cameraPosition)
	{
		//notice: probably not reliable to just == on float values but this is edge casey 
		//I belive if there is just a slight bit of variation this will be safe -- so == is probably okay.
		return;
	}

	//1. GET CURRENT VIEW VECTOR
	//this is reduce logic, the more repost is:
	// a. calculate a point in front of the camera (cameraPos + -w_axis)
	// b. get a vector to that point (point - cameraPos)
	// c. However, those step are superfluous as the vector is just -w_axis.
	vec3 currentLooking = -w_axis; //w_axis is a normalized vector
	

	//2. get a vector to the new look at position
	vec3 newLookAt = normalize(position - cameraPosition);

	//3. calculate the angle between the two vectors
	float cos_theta = dot(newLookAt, currentLooking);
	float theta_radians = acos(cos_theta); //imprecision can make values great than one for cos_theta; which acos(x>1) == nan
	if (isnan(theta_radians))
		return;

	//4. calculate the axis of rotation 
	//remember -- in right hand systems rotations occur in the way your fingers roll...
	//	a.place index finger in direction currently looking,
	//	b.place middle finger in direction of new looking
	//	c.thumb is axis of rotation
	//
	//we want to rotate toward the the new look at,
	//so our fingers should curl in that direction... 
	//this means we need to make it the second vector in our cross product
	//good measure to normalize this axis -- not sure if it is expected to be normalized with glm quaternion call
	if (vectorsAreSame(currentLooking, newLookAt)) //prevent nan within cross product for looking at same position
		return;
	vec3 rotationAxis = normalize(cross(currentLooking, newLookAt));

	//5. Calculation quaternion rotation
	quat q = angleAxis(theta_radians, rotationAxis);
	//6. stack ration onto current rotation
	quaternion = q * quaternion;

	//rebuild based on new quaternion
	updateAxes();
}

void FreeCamera::updateAxes()
{
	using namespace glm;

	mat4 transform = toMat4(quaternion);

	u_axis = vec3(transform * vec4(DEFAULT_AXIS_U, 0));
	v_axis = vec3(transform * vec4(DEFAULT_AXIS_V, 0));
	w_axis = vec3(transform * vec4(DEFAULT_AXIS_W, 0));

}

void FreeCamera::updateRoll(float rollAmount)
{
	using namespace glm;

	quat roll = angleAxis(radians(rollAmount), w_axis);
	quaternion = roll * quaternion;
	updateAxes();
}

void FreeCamera::mouseMovedPlanar(double xpos, double ypos)
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

void FreeCamera::mouseMovedDiscrete(double xpos, double ypos)
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
