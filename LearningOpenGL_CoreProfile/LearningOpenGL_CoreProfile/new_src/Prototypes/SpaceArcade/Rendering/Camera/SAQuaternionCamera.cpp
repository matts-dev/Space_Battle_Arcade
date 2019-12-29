#include "SAQuaternionCamera.h"
#include "../../Tools/SAUtilities.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/SAGameBase.h"

namespace SA
{
	QuaternionCamera::QuaternionCamera()
	{
		updateAxes();
	}

	void QuaternionCamera::updateAxes()
	{
		using namespace glm;

		mat4 transform = toMat4(quaternion);
		u_axis = normalize(vec3(transform * vec4(DEFAULT_AXIS_U, 0))); //#optimize the normalization of the basis may be superfluous, but needs testing due to floating point error
		v_axis = normalize(vec3(transform * vec4(DEFAULT_AXIS_V, 0)));
		w_axis = normalize(vec3(transform * vec4(DEFAULT_AXIS_W, 0)));

		//#TODO test and correct below #optimize Need to make sure my gut is right about the resulting matrix containing only transformed vecs
		//static const mat4 axesInColumns = mat4(vec4(DEFAULT_AXIS_U,0), vec4(DEFAULT_AXIS_V,0), vec4(DEFAULT_AXIS_W,0), vec4(0, 0, 0, 1));
		//mat4 transformedAxes = transform * axesInColumns;
		//u_axis = vec3(transformedAxes[0]); //#TODO consider whether these need to be normalized.
		//v_axis = vec3(transformedAxes[1]);
		//w_axis = vec3(transformedAxes[2]);
	}

	void QuaternionCamera::updateRoll(float rollAmount_rad)
	{
		using namespace glm;

		quat roll = angleAxis(rollAmount_rad, w_axis);
		quaternion = roll * quaternion;
		updateAxes();
	}

	void QuaternionCamera::lookAt_v(glm::vec3 point)
	{
		using namespace glm;

		vec3 currentLooking = -w_axis; //w_axis is a normalized vector
		vec3 newLookAt = normalize(point - getPosition());
		if (Utils::vectorsAreSame(currentLooking, newLookAt))
		{
			return;//prevent nan within cross product for looking at same position
		}

		float cos_theta = dot(newLookAt, currentLooking);
		float theta_radians = glm::clamp(acos(cos_theta), -1.f, 1.f); //imprecision can make values great than one for cos_theta; which acos(x>1) == nan
		if (isnan(theta_radians))
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "NAN encountered in look at");
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//calculate the axis of rotation 
		//	remember: in right-hand coordinate systems: rotations occur in the way your fingers roll when 
		//
		//	So, to understand the procedure we're about to do, do the following
		//		a.place your index finger in direction currently looking,
		//		b.place your middle finger in direction of new looking
		//		c.your thumb is the axis of rotation
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		vec3 rotationAxis = normalize(cross(currentLooking, newLookAt)); 
		quat q = angleAxis(theta_radians, rotationAxis);	//glm expects normalized axis.
		quaternion = q * quaternion;
		updateAxes(); //rebuild based on new quaternion
	}

	glm::mat4 QuaternionCamera::getView() const
	{
		return glm::lookAt(getPosition(), getPosition() + getFront(), getUp());
	}

	void QuaternionCamera::onMouseMoved_v(double xpos, double ypos)
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
		float pitchFactor = static_cast<float>(ypos - lastY);

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
		quat q1 = angleAxis(mouseSensitivity * rotationMagnitude, rotationAxis); //#TODO rotationMagnitude is adhoc, it needs to be converted to an angle

		//update the quaternion transform
		quaternion = q1 * quaternion;

		//update all axes
		updateAxes();

		lastX = xpos;
		lastY = ypos;

		//#TODO set this logging up behind compile time flag
		//std::cout << "u:" << u_axis.x << " " << u_axis.y << " " << u_axis.z << " ";
		//std::cout << "v:" << v_axis.x << " " << v_axis.y << " " << v_axis.z << " ";
		//std::cout << "w:" << w_axis.x << " " << w_axis.y << " " << w_axis.z << " " << std::endl;
	}

	void QuaternionCamera::onWindowFocusedChanged_v(int focusEntered)
	{
		//#TODO perhaps this functionality can be moved to base.
		CameraBase::onWindowFocusedChanged_v(focusEntered);
		refocused = focusEntered;
	}

	void QuaternionCamera::onMouseWheelUpdate_v(double xOffset, double yOffset)
	{
		float sensitivity = 0.5f;

		float& mutableFOV = adjustFOV();
		if (mutableFOV >= 1.0f && mutableFOV <= 45.0f)
			mutableFOV -= sensitivity * static_cast<float>(yOffset);
		if (mutableFOV <= 1.0f)
			mutableFOV = 1.0f;
		if (mutableFOV >= 45.0)
			mutableFOV = 45.0f;
	}

	void QuaternionCamera::tickKeyboardInput(float dt_sec)
	{
		//This is basic default movement, override this virtual to customize.

		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow();
		if (primaryWindow)
		{
			GLFWwindow* window = primaryWindow->get();

			float bSpeedAccerlationFactor = 1.0f;
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				bSpeedAccerlationFactor = 10.0f;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				adjustPosition(-(getFront() * freeRoamSpeed * bSpeedAccerlationFactor * dt_sec));
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			{
				adjustPosition(getFront() * freeRoamSpeed * bSpeedAccerlationFactor  * dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				//the w basis vector is the -cameraFront
				adjustPosition(getRight()* freeRoamSpeed * bSpeedAccerlationFactor  * dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				//the w basis vector is the -cameraFront
				glm::vec3 cameraRight = glm::normalize(glm::cross(getWorldUp_n(), -getFront()));
				adjustPosition(-(getRight()* freeRoamSpeed * bSpeedAccerlationFactor * dt_sec));
			}
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			{
				updateRoll(dt_sec * freeRoamRollSpeed_radSec);
			}
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			{
				updateRoll(dt_sec * -freeRoamRollSpeed_radSec);
			}
		}
	}

}

