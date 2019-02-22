#include "SATComponent.h"
#include <gtx\norm.hpp>



SATComponent::SATComponent()
{
}


SATComponent::~SATComponent()
{
}


/////////////////////////////////////////////////////////////////////////////////////

/*static*/ bool SATShape::CollisionTest(const SATShape& moving, const SATShape& stationary)
{
	glm::vec4 dummyMtv;
	return CollisionTest(moving, stationary, dummyMtv);
}

/*static*/ bool SATShape::CollisionTest(const SATShape& moving, const SATShape& stationary, glm::vec4& outMTV)
{
	//SAT collision test. 3d SAT requires not only the faces of the shape be tested, but
	//also to test edge x edge pairs. Below you will see we get axes for faces and edgexedge pairs.
	using std::vector; using glm::vec4; using glm::vec3;

	vector<vec3> normalizedAxes;
	normalizedAxes.clear();
	normalizedAxes.reserve(moving.faces.size() + stationary.faces.size() + moving.edges.size() * stationary.edges.size());

	moving.appendFaceAxes(normalizedAxes);
	stationary.appendFaceAxes(normalizedAxes);
	appendEdgeXEdgeAxes(moving, stationary, normalizedAxes);

	vec3 mtv(0.0f);		//mtv = minimum translation vector to get out of collision
	for (const vec3& axis : normalizedAxes)
	{
		SAT::ProjectionRange movProj = moving.projectToAxis(axis);
		SAT::ProjectionRange staProj = stationary.projectToAxis(axis);

		bool disjoint = movProj.max < staProj.min || staProj.max < movProj.min;
		if (disjoint)
		{
			outMTV = vec4(0.0f);
			return false;
		}
		else /*vectors are known to overlap (not disjoint); certain assumptions can be made (eg no 0 len MTV)*/
		{
			glm::vec3 candidateMTV = calculateMinimumTranslationVec(axis, movProj, staProj);
			float newMTVLength2 = glm::length2(candidateMTV);
			float oldMTVLength2 = glm::length2(mtv);

			//float zero comparision should be safe in this case; it is just to catch first MTV
			if (newMTVLength2 < oldMTVLength2 || oldMTVLength2 == 0.0f)
			{
				mtv = candidateMTV;
			}
		}
	}

	outMTV = vec4(mtv, 0.0f);
	outMTV *= floatMTVCorrectionFactor;
	return true;
}

SATShape::SATShape(
	const std::vector<glm::vec4>& inLocalPoints,
	const std::vector<EdgePointIndices>& edgeIdxs,
	const std::vector<FacePointIndices>& faceIdxs)
{
	localPoints = inLocalPoints;

	//precreate space and objects that will update edge/face refs; transform applied later
	transformedPoints = inLocalPoints;
	
	for (const EdgePointIndices& edgeIdx: edgeIdxs)
	{
		edges.emplace_back(
			transformedPoints[edgeIdx.indexA],
			transformedPoints[edgeIdx.indexB]
		);
	}

	for (const FacePointIndices& faceIdx : faceIdxs)
	{
		faces.emplace_back(
			transformedPoints[faceIdx.edge1.indexA],
			transformedPoints[faceIdx.edge1.indexB],
			transformedPoints[faceIdx.edge2.indexA],
			transformedPoints[faceIdx.edge2.indexB]
		);
	}

}

void SATShape::updateTransform(const glm::mat4& inTransform)
{
	transform = inTransform;
	applyTransform();
}

void SATShape::applyTransform()
{
	for (uint32_t pnt = 0; pnt < localPoints.size(); ++pnt)
	{
		transformedPoints[pnt] = transform * localPoints[pnt];
	}
	transformedOrigin = transform * localOrigin;
}

void SATShape::appendFaceAxes(std::vector<glm::vec3>& outAxes) const
{
	using glm::vec3; using glm::vec4;
	for (const SATFaceRef& face : faces)
	{
		vec4 e1 = face.edge1.pntA - face.edge1.pntB;
		vec4 e2 = face.edge2.pntA - face.edge2.pntB;
		vec3 axis = glm::normalize(glm::cross(vec3(e1), vec3(e2)));
		outAxes.push_back(axis);
	}
}

void SATShape::appendEdgeXEdgeAxes(const SATShape& moving, const SATShape& stationary, std::vector<glm::vec3>& normalizedAxes)
{
	using std::vector; using glm::vec4; using glm::vec3;

	for (const SATEdgeRef& moveEdgeRef : moving.edges)
	{
		for (const SATEdgeRef& stationaryEdgeRef : stationary.edges)
		{
			vec3 movEdge = moveEdgeRef.pntA - moveEdgeRef.pntB;
			vec3 statEdge = stationaryEdgeRef.pntA - stationaryEdgeRef.pntB;
			//direction of cross product doesn't matter; projections will be consistent
			vec3 axis = glm::normalize(cross(movEdge, statEdge));
			normalizedAxes.push_back(axis);
		}
	}
}

SAT::ProjectionRange SATShape::projectToAxis(const glm::vec3& normalizedAxis) const
{
	using glm::vec3; using glm::vec4;

	SAT::ProjectionRange projRange;

	for (const glm::vec4& pnt4 : transformedPoints)
	{
		//project point onto the axis
		vec3 pnt3 = vec3(pnt4);
		float projection = glm::dot(pnt3, normalizedAxis);

		projRange.max = projRange.max > projection ? projRange.max : projection;
		projRange.min = projRange.min < projection ? projRange.min : projection;
	}

	return projRange;
}

void SATShape::overrideLocalOrigin(glm::vec4 newLocalOriginPoint)
{
	localOrigin = newLocalOriginPoint;
	applyTransform();
}

/*static*/ glm::vec3 SATShape::calculateMinimumTranslationVec(const glm::vec3& unitAxis, const SAT::ProjectionRange& movingProj, const SAT::ProjectionRange& stationaryProj)
{
	//INVARIANT: the two projects are not disjoint, and there is known overlap.
	//	this method assumes that the collision test has already failed;
	//	otherwise it will return invalid minimum translation vectors (MTV)s for disjoint axis

	const SAT::ProjectionRange& m = movingProj;
	const SAT::ProjectionRange& s = stationaryProj;

	//below are some images to compare the logic against
	// |---| = m	(---) = s
	//
	// |--(--|---)			//m on left of s; 
	//
	//    (--|---)--|		//m on right of s
	//
	//  (-|----|-)			//m contained in s
	//
	//	|--(---)--|			//m contains s
	bool movOnLeft = m.max >= s.min && m.max <= s.max;		
	bool movOnRight = m.min <= s.max && m.min >= s.min;		

	bool movInside = movOnLeft && movOnRight;
	bool staInside = !movInside && !movOnLeft && !movOnRight; //relies on invariant

	if (movInside || staInside)
	{
		const SAT::ProjectionRange& BiggerObject = movInside ? stationaryProj : movingProj;
		const SAT::ProjectionRange& InsideObject = movInside ? movingProj : stationaryProj;

		float rightDist = BiggerObject.max - InsideObject.max;
		float leftDist = BiggerObject.min - InsideObject.min;

		//MTV is in direction of shorter distance; but include full size of inner object
		//  (-|----|------)			
		//  <--
		if (rightDist > leftDist) //move left (negative)
		{
			return unitAxis * -(leftDist + (InsideObject.max -InsideObject.min));
		}
		else if (rightDist < leftDist) //move right (positive)
		{
			return unitAxis * (rightDist + (InsideObject.max - InsideObject.min));
		}
		else //two are perfectly aligned; move full distance
		{
			//arbitrary direction since perfect alignment; both choices valid; must move full size
			return (BiggerObject.max - BiggerObject.min) * unitAxis;
		}
	}
	else if(movOnLeft)
	{
		// |--(--|---)
		//    <--  vector needed
		return (s.min - m.max) * unitAxis;
	}
	else //movOnRight
	{
		//(--|---)--|		
		//   ---->  vector needed
		return (s.max - m.min) * unitAxis;
	}
}

/////////////////////////////////////////////////////////////////////////////////////

/*static*/ const std::vector<glm::vec4> SATCubeShape::shapePnts =
{
	//the front face and back face contain all points in the cube
	//front face
	glm::vec4(0.5f,  -0.5f,  0.5f,  1.0f),	//F-RB (Front-RightBottom)
	glm::vec4(0.5f,  0.5f,   0.5f,  1.0f),	//F-RT
	glm::vec4(-0.5f, 0.5f,   0.5f,  1.0f),	//F-LT
	glm::vec4(-0.5f, -0.5f,  0.5f,  1.0f),	//F-LB

	//back face
	glm::vec4(0.5f,  -0.5f,  -0.5f,  1.0f), //B-RB
	glm::vec4(0.5f,  0.5f,   -0.5f,  1.0f), //B-RT
	glm::vec4(-0.5f, 0.5f,   -0.5f,  1.0f), //B-LT
	glm::vec4(-0.5f, -0.5f,  -0.5f,  1.0f)  //B-LB
};

/*static*/ const std::vector<SATShape::EdgePointIndices> SATCubeShape::edgePntIndices = 
{
	//the cube only really has 3 unique edges (to visualize: imagine the unit vectors in 1 corner)
	{1, 0},	//RT <= RB edge
	{3, 0},	//LB <= RB edge
	{4, 0}	//B-RB <= F-RB edge 
};

/*static*/ const std::vector<SATShape::FacePointIndices> SATCubeShape::facePntIndices =
{
	//All of the following are assumed to be in right handed coordinates;
	//though it should not matter as the axis is the same regardless of direction of normal vector

	//front face (same axis as back face)
	{
		{1,0}, //F-RT <= F-RB
		{3,0}, //F-LB <= F-RB
	},
	//left face (same axis as right face)
	{
		{4,0}, //B-LB <= F-RB 
		{1,0}  //F-RT <= F-RB
	},
	//top face (same axis as bottom face)
	{
		{5,1},	//B-RT <= F-RT
		{2,1}	//F-LT <= F-RT
	}
};

SATCubeShape::SATCubeShape() :
	SATShape(shapePnts, edgePntIndices, facePntIndices)
{

}


SATShape2D::ConstructHelper::ConstructHelper(const std::vector<glm::vec2>& convexPoints)
{
	//Create an additional layer so that perpendicular axes can be calculated from faces
	//side view of the 2d planes:
	// ac----bc----cc		//copied layer top
	// | F1  |  F2 |		//face's F1 and F2 normals can be used as a perpendicular
	// a-----b-----c		//original points make flat plane
	//since we're in 2d, we don't need to worry about 3d edge x edge detection
	faces.clear();
	points4.clear();

	faces.reserve(sizeof(convexPoints));
	points4.reserve(2 * sizeof(convexPoints));

	//construct the two layers
	for (const glm::vec2& PointForBottom : convexPoints)
	{
		points4.push_back(glm::vec4(PointForBottom, 0, 1));
	}
	for (const glm::vec2& PointForTop : convexPoints)
	{
		points4.push_back(glm::vec4(PointForTop, 1, 1));
	}

	//calculate face indices so we can use the 3d SAT face axis calculations
	size_t num2DPnts = convexPoints.size();
	for (size_t edgeStartIdx = 0; edgeStartIdx < num2DPnts; ++edgeStartIdx)
	{
		size_t horizontalPointIdx = (edgeStartIdx + 1) % num2DPnts;		//wrap around for last index
		size_t verticalPointIdx = edgeStartIdx + num2DPnts;				//this reach into top layer indices

		SATShape2D::FacePointIndices face;
		face.edge1.indexA = horizontalPointIdx;
		face.edge1.indexB = edgeStartIdx;
		face.edge2.indexA = verticalPointIdx;
		face.edge2.indexB = edgeStartIdx;	

		faces.push_back(face);
	}
}

SATShape2D::SATShape2D(const ConstructHelper& constructInfo)
	: SATShape(constructInfo.points4, {/*empty_vector; edges x edge for 2d*/ }, constructInfo.faces)
{

}