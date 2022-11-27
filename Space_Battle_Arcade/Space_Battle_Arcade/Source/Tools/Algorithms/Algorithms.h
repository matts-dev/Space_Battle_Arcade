#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <limits>
#include <algorithm>

namespace SA
{

	struct Triangle
	{
		glm::vec3 pntA;
		glm::vec3 pntB;
		glm::vec3 pntC;
	};

	struct Ray
	{
		//ray = start + T * dir
		float T = 1.0f;
		glm::vec3 start = glm::vec3(0, 0, 0);
		glm::vec3 dir = glm::vec3(1, 0, 0);

		void initForIntersectionTests()
		{
			T = std::numeric_limits<float>::infinity();
		}
		bool triangleTest(const Triangle& tri, glm::vec3& intersectPnt, bool bUpdateT = true)
		{
			//Invariant: this should be a valid triangle such that the cross of its edges does not produce NAN/zerovector results

			using glm::vec3; using glm::vec4; using glm::normalize; using glm::cross; using glm::dot;

			//PARAMETRIC EQ FOR PLANE: F(u,v) : (u*edgeA_n + v*edgeB_n) 
			//IMPLICIT EQ FOR PLANE:   F(pnt) : dot( (pnt - tri.pntA) , triNormal) = 0
			//create parametric equation for plane of triangle
			const vec3 edgeA = vec3(tri.pntA - tri.pntB);
			const vec3 edgeB = vec3(tri.pntA - tri.pntC);
			const vec3 planeNormal_n = normalize(cross(edgeA, edgeB));

			//choose a point in plane that is not the same as the start of the ray
			vec3 planePnt = tri.pntA;
			planePnt = length(planePnt - start) > 0.0001 ? tri.pntB : planePnt;


			// calculate ray T value when it intersects plane triangle is within
			//		notation: @ == dot product; capital_letters == vector; S = rayStart, D = rayDirection, B = pointInPlane,
			//		ray-plane intersection:
			//			N @ ((S - tD) - B) = 0
			//			N@S - N@tD - N@b = 0			//distribute dot
			//			N@tD = -N@S + N@B
			//			t(N@D) = -N@S + N@B				//factor out t
			//			t = (-N@S + N@B) / (N@D)		//divide over dot-product coefficient
			//			t = (N@(-S+B) / N@D)			//factor out N from numerator
			//			t = (N@(B-S) / N@D)				//rearrange 

			//check that ray is not in the plane of the triangle
			vec3 dir_n = normalize(dir);
			vec3 s_to_b = vec3(planePnt) - start;
			bool bRayInPlane = glm::abs(dot(planeNormal_n, dir_n)) < 0.001;
			if (bRayInPlane || s_to_b == vec3(0.f)) return false; //do not consider this a collision

			//calculate t where ray passes through triangle's plane
			float newT = dot(planeNormal_n, s_to_b) / dot(planeNormal_n, dir_n);
			//do not consider larger t values, or negative t values (negative t values will intersect with objects behind camera)
			if (newT > T || newT < 0.0f) return false;
			const vec3 rayPnt = start + newT * dir_n;

			//check if point is within the triangle using crossproduct comparisons with normal
			// X =crossproduct, @=dotproduct
			//   C
			//  /  \      p_out
			// / p   \    
			//A-------B
			//
			//planeNormal = (B-A) X(C-A)
			//
			// verify the below by using right hand rule for crossproducts
			// p_out should return a vector opposite to the normal when compared with edge CB 
			// dot is positive(+) when they're the same, dot is negative(-) when they're opposite
			//
			//edge AB test = ((B-A) X (p-A)) @ (planeNormal) > 0
			//edge BC test = ((C-B) X (p-B)) @ (planeNormal) > 0
			//edge CA test = ((A-C) X (p-C)) @ (planeNormal) > 0
			bool bBA_Correct = dot(cross(tri.pntB - tri.pntA, rayPnt - tri.pntA), planeNormal_n) > 0;
			bool bCB_Correct = dot(cross(tri.pntC - tri.pntB, rayPnt - tri.pntB), planeNormal_n) > 0;
			bool bAC_Correct = dot(cross(tri.pntA - tri.pntC, rayPnt - tri.pntC), planeNormal_n) > 0;
			if (!bBA_Correct || !bCB_Correct || !bAC_Correct)
			{
				return false;
			}

			T = newT;
			intersectPnt = rayPnt;
			return true;
		}
	};

	class ObjectPicker
	{
	public:
		static Ray generateRay(
			const glm::vec2& screenResolution, const glm::vec2& clickPoint,
			const glm::vec3& cameraPos_w, const glm::vec3& cameraUp_n, const glm::vec3& cameraRight_n, const glm::vec3& cameraFront_n,
			float FOVy, float aspectRatio
		)
		{
			//convert click point to NDC
			glm::vec2 ndcClick = glm::vec2{
				clickPoint.x / screenResolution.x,
				clickPoint.y / screenResolution.y
			};					//range [0, 1]
			//make this relative to center of screen, not top corner
			//-0.5 is an offset that transforms left-top points to center points (math works out)
			//0.1 - 0.5f = -0.4; 0.6-0.5 = 0.1; 1.0 - 0.5f 0.5f; so this limits the range to [-0.5, 0.5]
			ndcClick += glm::vec2(-0.5f);	//range [-0.5, 0.5]
			ndcClick *= 2.0f;				//range [-1,1]
			ndcClick.y *= -1.0f;
			//       x
			//______________   RENDER PLANE 
			//   |       /
			//   |     /
			// z |FOVx/
			//   |  /
			//   |/
			//   C---------->
			//		(r_v)
			// Z = depth from camera to render plane; known
			// r_v = camera's right vector; known
			// x = scalar for r_v to reach maximum visble area pased on FOV; unknown
			//
			// tan(FOVx) = x / z
			//  z * tan(FOVx) = x
			float hFOVy = glm::radians(FOVy / 2.0f);
			float tan_hFOVy = glm::tan(hFOVy);

			//assuming dist to plane (z) is 1
			float upScalar = tan_hFOVy* ndcClick.y;
			glm::vec3 up = cameraUp_n * upScalar;

			//again, assuming z is 1;
			float rightScalar = aspectRatio * tan_hFOVy * ndcClick.x;
			glm::vec3 right = cameraRight_n * rightScalar;

			//this is relative to the origin, but we can just add cameraPos to get this point in world space
			glm::vec3 click_dir = (up + right + cameraFront_n);

			Ray rayFromCamera;
			rayFromCamera.start = cameraPos_w;
			rayFromCamera.dir = glm::normalize(click_dir);
			rayFromCamera.T = glm::length(click_dir);
			return rayFromCamera;
		}
	};


}