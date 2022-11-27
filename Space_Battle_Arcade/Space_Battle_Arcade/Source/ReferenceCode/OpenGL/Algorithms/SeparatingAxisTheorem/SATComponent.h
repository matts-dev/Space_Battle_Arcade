#pragma once
#include <vector>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SAT
{

	/** 
		Represents a component that can do separating axis theorem collision calculations

		Assumes column vectors used; code may need to be tweaked (or may not) for row vectors.
		Assumes right handed cross products; which are used extensively for face and axis calculations
	*/

/////////////////////////////////////////////////////////////////////////////////////

	/** 
		Edge is defined as the vector pntA - pntB.

		memory should be managed by owning SATShape.
	*/
	struct EdgeRef
	{
		EdgeRef(const glm::vec4& inPntA, const glm::vec4& inPntB) 
			: pntA(inPntA), pntB(inPntB) {}

		const glm::vec4& pntA;
		const glm::vec4& pntB;
	};

	/** 
		face normal is defined by edge1 cross edge2.
		edge is defined as the vector pntA - pntB.

		memory should be managed by owning SATShape.
	*/
	struct FaceRef
	{
		FaceRef(
			const glm::vec4& edge1_pntA, const glm::vec4& edge1_pntB, 
			const glm::vec4& edge2_pntA, const glm::vec4& edge2_pntB)
			:	edge1(edge1_pntA, edge1_pntB),
				edge2(edge2_pntA, edge2_pntB)
		{}

		const EdgeRef edge1;
		const EdgeRef edge2;
	};

/////////////////////////////////////////////////////////////////////////////////////


	struct ProjectionRange
	{
		float min = std::numeric_limits<float>::infinity();
		float max = -std::numeric_limits<float>::infinity();
	};


	/**
		Represents a shape (cube, polyhedron capsule, etc.)
		When transform is updated, all local points are transformed.

		Edge and faces are derived from vectors created from transform points.
		Non-uniform scales are safe since axis vectors are derivations from transformed points.

		For now, this class is defined without virtuals for speed; if virtuals are added please mark dtor
		virtual and remove this last sentence.
	 */
	class Shape
	{
	public: //ctor arguments; these correspond to the faces and edge structs
		struct EdgePointIndices 
		{ 
			uint32_t indexA;
			uint32_t indexB;
		};
		struct FacePointIndices
		{
			EdgePointIndices edge1;
			EdgePointIndices edge2;
		};
	public: //statics
		/** 
			Returns true if collision occured; minimum translation vector(mtv) returned as out variable.
			MTV is a vector is the direction to translate the moving SATShape so that it is no longer colliding.
		*/
		static bool CollisionTest(const Shape& moving, const Shape& stationary);
		static bool CollisionTest(const Shape& moving, const Shape& stationary, glm::vec4& outMTV);
		static constexpr float floatMTVCorrectionFactor = 1.1f;

	public: //members
		Shape(
			const std::vector<glm::vec4>& inLocalPoints, 
			const std::vector<EdgePointIndices>& edgeIdxs,
			const std::vector<FacePointIndices>& faceIdxs);
		void updateTransform(const glm::mat4& inTransform);
		void applyTransform();

		void appendFaceAxes(std::vector<glm::vec3>& outAxes) const;
		static void appendEdgeXEdgeAxes(const Shape& moving, const Shape& stationary, std::vector<glm::vec3>& normalizedAxes);
		SAT::ProjectionRange projectToAxis(const glm::vec3& normalizedAxis) const;

		/* Not provided in ctor because normally the origin will be at the center of shapes; see default value*/
		void overrideLocalOrigin(glm::vec4 newLocalOriginPoint);
		glm::vec4 getTransformedOrigin() const { return transformedOrigin; }

	private:
		/** INVARIANT: Unit Axis is a normalized vector;INVARIANT: The two projections are not disjoint	*/
		static glm::vec3 calculateMinimumTranslationVec(const glm::vec3& unitAxis, const SAT::ProjectionRange& movingProj, const SAT::ProjectionRange& stationaryProj);

	public: //debugging helpers; provided to allow visualization of collision shapes as points and visual unique edges/faces
		/** The following are debug methods are debug methods and not intended for normal SAT usage*/
		const std::vector<glm::vec4>& getTransformedPoints() const { return transformedPoints; };
		const std::vector<glm::vec4>& getLocalPoints() const { return localPoints; }
		const std::vector<EdgePointIndices>& getDebugEdgeIdxs() const { return cachedDebugEdgeIdxs; }
		const std::vector<FacePointIndices>& getDebugFaceIdxs() const { return cachedDebugFaceIdxs; };
	private: //cached debug state
		const std::vector<EdgePointIndices> cachedDebugEdgeIdxs;
		const std::vector<FacePointIndices> cachedDebugFaceIdxs;

	private:
		//When a new transform is set, the transformed points are updated by transforming the local points
		//The face and edges of class hold references to the transformed points, so their data is always valid for calculations

		//NOTE: It is imperative that these are private-scope. faces and edges rely on references to contents of transformed points.
		//if transformedPoints (or localPoints) vectors are changed after construction, will invalid references in faces and edges
		glm::mat4 transform;
		std::vector<glm::vec4> localPoints;
		std::vector<glm::vec4> transformedPoints;
		glm::vec4 localOrigin = glm::vec4(0, 0, 0, 1);
		glm::vec4 transformedOrigin;
		std::vector<FaceRef> faces;
		std::vector<EdgeRef> edges;

	};


/////////////////////////////////////////////////////////////////////////////////////

	class Shape2D : public Shape
	{
	public:
		/*Shape2d requires data in initializer list; this helper make data available before ctor body*/
		struct ConstructHelper
		{
			ConstructHelper(const std::vector<glm::vec2>& convexPoints);
		private: 
			friend Shape2D;
			std::vector<FacePointIndices> faces;
			std::vector<glm::vec4> points4;
		};

	public:
		Shape2D(const ConstructHelper& constructInfo);

	private:
	};

	/////////////////////////////////////////////////////////////////////////////////////

	class CubeShape : public Shape
	{
	public: /*statics defining geometry of cube shape*/
		static const std::vector<glm::vec4> shapePnts;
		static const std::vector<EdgePointIndices> edgePntIndices;
		static const std::vector<FacePointIndices> facePntIndices;

	public:
		CubeShape();

	private:

	};

	/////////////////////////////////////////////////////////////////////////////////////

	class PolygonCapsuleShape : public Shape
	{
		// Side View Of Capsule; only two points are not visible from this view.
		// Points are represented by X or &
		// The obscured points are directly behind the points marked as a &
		//   X -- x
		//  /|    |\
		// x-&----&-x
		//  \|    |/
		//   x -- x
	public: /*statics defining geometry of polygon capsule shape*/
		static const std::vector<glm::vec4> shapePnts;
		static const std::vector<EdgePointIndices> edgePntIndices;
		static const std::vector<FacePointIndices> facePntIndices;

	public:
		PolygonCapsuleShape();

	private:

	};


	/////////////////////////////////////////////////////////////////////////////////////

	class DynamicTriangleMeshShape : public Shape
	{
	public:
		class TriangleProcessor
		{
		public:
			struct TriangleCCW
			{
				//points in local space
				glm::vec4 pntA;
				glm::vec4 pntB;
				glm::vec4 pntC;
			};
			TriangleProcessor(const std::vector<TriangleCCW>& triangles, float considerDotsSameIfWithin);
		private:
			friend DynamicTriangleMeshShape;
			std::vector<glm::vec4> points;
			std::vector<EdgePointIndices> edgeIndices;
			std::vector<FacePointIndices> faceIndices;
		};


	public:
		DynamicTriangleMeshShape(const TriangleProcessor& PreprocessedTriangles);

	private:

	};

};