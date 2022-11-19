#include "SATComponent.h"
#include <gtx/norm.hpp>

namespace SAT
{


/////////////////////////////////////////////////////////////////////////////////////


	/*static*/ bool Shape::CollisionTest(const Shape& moving, const Shape& stationary)
	{
		glm::vec4 dummyMtv;
		return CollisionTest(moving, stationary, dummyMtv);
	}

	/*static*/ bool Shape::CollisionTest(const Shape& moving, const Shape& stationary, glm::vec4& outMTV)
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

	Shape::Shape(
		const std::vector<glm::vec4>& inLocalPoints,
		const std::vector<EdgePointIndices>& edgeIdxs,
		const std::vector<FacePointIndices>& faceIdxs) 
		: cachedDebugFaceIdxs(faceIdxs), cachedDebugEdgeIdxs(edgeIdxs)
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

	void Shape::updateTransform(const glm::mat4& inTransform)
	{
		transform = inTransform;
		applyTransform();
	}

	void Shape::applyTransform()
	{
		for (uint32_t pnt = 0; pnt < localPoints.size(); ++pnt)
		{
			transformedPoints[pnt] = transform * localPoints[pnt];
		}
		transformedOrigin = transform * localOrigin;
	}

	void Shape::appendFaceAxes(std::vector<glm::vec3>& outAxes) const
	{
		using glm::vec3; using glm::vec4;
		for (const FaceRef& face : faces)
		{
			vec4 e1 = face.edge1.pntA - face.edge1.pntB;
			vec4 e2 = face.edge2.pntA - face.edge2.pntB;
			vec3 axis = glm::normalize(glm::cross(vec3(e1), vec3(e2)));
			outAxes.push_back(axis);
		}
	}

	void Shape::appendEdgeXEdgeAxes(const Shape& moving, const Shape& stationary, std::vector<glm::vec3>& normalizedAxes)
	{
		using std::vector; using glm::vec4; using glm::vec3;

		for (const EdgeRef& moveEdgeRef : moving.edges)
		{
			for (const EdgeRef& stationaryEdgeRef : stationary.edges)
			{
				vec3 movEdge = moveEdgeRef.pntA - moveEdgeRef.pntB;
				vec3 statEdge = stationaryEdgeRef.pntA - stationaryEdgeRef.pntB;
				//direction of cross product doesn't matter; projections will be consistent
				vec3 axis = glm::normalize(cross(movEdge, statEdge));
				if (!isnan(axis.x) && !isnan(axis.y) && !isnan(axis.z))
				{
					normalizedAxes.push_back(axis);
				}
			}
		}
	}

	SAT::ProjectionRange Shape::projectToAxis(const glm::vec3& normalizedAxis) const
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

	void Shape::overrideLocalOrigin(glm::vec4 newLocalOriginPoint)
	{
		localOrigin = newLocalOriginPoint;
		applyTransform();
	}

	/*static*/ glm::vec3 Shape::calculateMinimumTranslationVec(const glm::vec3& unitAxis, const SAT::ProjectionRange& movingProj, const SAT::ProjectionRange& stationaryProj)
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
		
			//determine which direction to slide smaller object out of bigger object (think of this as creating vectors in 1d)
			//check which distance is less to travel; this may need abs(), but I think it can be mathematically shown 
			//that the max diff is always positive and the min diff is always negative. There are 3 case for each
			// case 1: values straddle 0			{--0--}		| mins: neg - pos			= neg	|maxs: pos - neg			= pos
			// case 2: values are on negative side  {----} 0	| mins: big_neg - small_neg = neg	|maxs: small_neg - big_neg	= pos
			// case 3: values are on positive side  0 {----}	| mins: small - large		= neg	|maxs: big_pos - small_pos	= pos
			//saving cos of abs() by not using it.
			float rightDist = BiggerObject.max - InsideObject.max;		//always a positive, (see above table)
			float leftDist = -(BiggerObject.min - InsideObject.min);	//always a negative number (see above table)

			//MTV is in direction of shorter distance; but include full size of inner object
			//  (-|----|------)			
			//  <--
			glm::vec3 mtv(0.0f);
			if (rightDist > leftDist) //move left (negative)
			{
				mtv = unitAxis * -(leftDist + (InsideObject.max -InsideObject.min));
			}
			else if (rightDist < leftDist) //move right (positive)
			{
				mtv = unitAxis * (rightDist + (InsideObject.max - InsideObject.min));
			
			}
			else //two are perfectly aligned; move full distance
			{
				//arbitrary direction since perfect alignment; both choices valid; must move full size
				mtv = (BiggerObject.max - BiggerObject.min) * unitAxis;
			}

			//In cases moveable object has moved to now surround the stationary object, we will return the mtv for the stationary object
			//we can correct this by inverting the mtv.
			// |---------------(---)---|
			//					------->	//mtv from calculated above
			//					<------     //mtv inverted so the movable |--| object is moved and not the stationary object
			return movInside ? mtv : -mtv;
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

	/*static*/ const std::vector<glm::vec4> CubeShape::shapePnts =
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

	/*static*/ const std::vector<Shape::EdgePointIndices> CubeShape::edgePntIndices = 
	{
		//the cube only really has 3 unique edges (to visualize: imagine the unit vectors in 1 corner)
		{1, 0},	//RT <= RB edge
		{3, 0},	//LB <= RB edge
		{4, 0}	//B-RB <= F-RB edge 
	};

	/*static*/ const std::vector<Shape::FacePointIndices> CubeShape::facePntIndices =
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

	CubeShape::CubeShape() :
		Shape(shapePnts, edgePntIndices, facePntIndices)
	{

	}

/////////////////////////////////////////////////////////////////////////////////////////

	/*static*/ const std::vector<glm::vec4> PolygonCapsuleShape::shapePnts =
	{
		//bottom tip (-y)
		/*0*/glm::vec4(0, -1.5f, 0, 1),

		//bottom side ring (-y)
		/*1*/glm::vec4(1, -0.5f, 0, 1),		//median point
		/*2*/glm::vec4(0, -0.5f, 1, 1),		//top point
		/*3*/glm::vec4(-1,-0.5f, 0, 1),		//median point
		/*4*/glm::vec4(0, -0.5f, -1, 1),	//bottom point

		//stop side ring (+y)
		/*5*/glm::vec4(1,  0.5f, 0, 1),		//median point
		/*6*/glm::vec4(0,  0.5f, 1, 1),		//top point
		/*7*/glm::vec4(-1, 0.5f, 0, 1),		//median point
		/*8*/glm::vec4(0,  0.5f, -1, 1),	//bottom point

		//top tip (+y)
		/*9*/glm::vec4(0, 1.5f, 0, 1)
	};

	/*static*/ const std::vector<Shape::EdgePointIndices> PolygonCapsuleShape::edgePntIndices =
	{
		/* These are only the unique edges, ignoring parallel edges*/
		{0, 1},
		{0, 2},
		{0, 3},
		{0, 4},

		{2, 1 },
		{4, 1 },
		//{2, 3 }, same as {4,1}
		//{4, 3 }, same as {2, 1}

		{1, 5 },

		//edges to 9 are redundant with edges to 0
	};

	/*static*/ const std::vector<Shape::FacePointIndices> PolygonCapsuleShape::facePntIndices =
	{
		//corner faces (these are redundant with other corner)
		{
			{1,0},
			{2, 0}
		},
		{
			{2,0},
			{3, 0}
		},
		{
			{3,0},
			{4,0}
		},
		{
			{4,0},
			{1,0}
		},

		//side faces (redundant face mirror across y axis)
		{
			{5,1},
			{2,1}
		},
		{
			{4,1},
			{5,1}
		},
	};

	PolygonCapsuleShape::PolygonCapsuleShape() :
		Shape(shapePnts, edgePntIndices, facePntIndices)
	{

	}

/////////////////////////////////////////////////////////////////////////////////////////

	Shape2D::ConstructHelper::ConstructHelper(const std::vector<glm::vec2>& convexPoints)
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

			Shape2D::FacePointIndices face;
			face.edge1.indexA = horizontalPointIdx;
			face.edge1.indexB = edgeStartIdx;
			face.edge2.indexA = verticalPointIdx;
			face.edge2.indexB = edgeStartIdx;	

			faces.push_back(face);
		}
	}

	Shape2D::Shape2D(const ConstructHelper& constructInfo)
		: Shape(constructInfo.points4, {/*empty_vector; edges x edge for 2d*/ }, constructInfo.faces)
	{

	}

/////////////////////////////////////////////////////////////////////////////////////////


	DynamicTriangleMeshShape::DynamicTriangleMeshShape(const TriangleProcessor& PreprocessedTriangles)
		:Shape(PreprocessedTriangles.points, PreprocessedTriangles.edgeIndices, PreprocessedTriangles.faceIndices)
	{
	}

	DynamicTriangleMeshShape::TriangleProcessor::TriangleProcessor(const std::vector<TriangleCCW>& triangles, float considerDotsSameIfWithin)
	{
		using glm::cross; using glm::normalize; using glm::vec3; using glm::vec4; using glm::dot;

		//used to reduce the number of std::vector space reserved, since symmetric objects will hopefully have redundant edges/faces
		const size_t mirrorRedundancyHeuristic = 2;

		std::vector<vec3> uniqueFaceNormals;
		uniqueFaceNormals.reserve(triangles.size());

		std::vector<vec3> uniqueEdges;
		uniqueEdges.reserve(triangles.size() / mirrorRedundancyHeuristic); //just a guess heuristic to prevent early buffer resizes

		points.reserve(triangles.size() * 3);
		faceIndices.reserve(triangles.size() / mirrorRedundancyHeuristic);
		edgeIndices.reserve((3 * triangles.size()) / mirrorRedundancyHeuristic);

		for (const TriangleCCW& tri : triangles)
		{
			points.push_back(tri.pntA);
			points.push_back(tri.pntB);
			points.push_back(tri.pntC);
			size_t aIdx = points.size() - 3;
			size_t bIdx = points.size() - 2;
			size_t cIdx = points.size() - 1;

			float vecsSameIfGreaterOrEqualThanThis = 1.0f - considerDotsSameIfWithin;

			//Test if face has unique normal; if so we need to use it as an axis of separation
			vec3 faceNormal = normalize(cross(vec3(tri.pntC - tri.pntB), vec3(tri.pntA - tri.pntB)));
			bool faceIsUnique = true;
			for (vec3 prevNormal : uniqueFaceNormals)
			{
				//we take absolute value because both sign directions of separating axis yield same result
				float relatedness = glm::abs(dot(prevNormal, faceNormal));
				if (relatedness >= vecsSameIfGreaterOrEqualThanThis)
				{
					faceIsUnique = false;
					break;
				}
			}
			if (faceIsUnique)
			{
				uniqueFaceNormals.push_back(faceNormal);
				EdgePointIndices edge1{ cIdx, bIdx};
				EdgePointIndices edge2{ aIdx, bIdx };
				faceIndices.push_back(FacePointIndices{ EdgePointIndices{ cIdx, bIdx}, EdgePointIndices{ aIdx, bIdx } });
			}

			//test if edges are unique
			vec3 edgeCB_n = normalize(tri.pntC - tri.pntB); 
			vec3 edgeAB_n = normalize(tri.pntA - tri.pntB);
			vec3 edgeCA_n = normalize(tri.pntC - tri.pntA);

			//repeated code to optimize for single loop
			bool CBUnique, ABUnique, CAUnique;
			CBUnique = ABUnique = CAUnique = true;
			for (const vec3& prevEdge_n : uniqueEdges)
			{
				//if non unique, update, else passthrough previous value
				CBUnique = glm::abs(dot(prevEdge_n, edgeCB_n)) >= vecsSameIfGreaterOrEqualThanThis ? false : CBUnique;
				ABUnique = glm::abs(dot(prevEdge_n, edgeAB_n)) >= vecsSameIfGreaterOrEqualThanThis ? false : ABUnique;
				CAUnique = glm::abs(dot(prevEdge_n, edgeCA_n)) >= vecsSameIfGreaterOrEqualThanThis ? false : CAUnique;

				//early end loop if all edges have been accounted for already
				if (!CAUnique && !ABUnique && !CAUnique){ break;}
			}
			//add new valid edges to shape
			if (CBUnique) { uniqueEdges.push_back(edgeCB_n); edgeIndices.emplace_back(EdgePointIndices{ cIdx, bIdx }); }
			if (ABUnique) { uniqueEdges.push_back(edgeAB_n); edgeIndices.emplace_back(EdgePointIndices{ aIdx, bIdx });}
			if (CAUnique) { uniqueEdges.push_back(edgeCA_n); edgeIndices.emplace_back(EdgePointIndices{ cIdx, aIdx });}
		}
	}
}