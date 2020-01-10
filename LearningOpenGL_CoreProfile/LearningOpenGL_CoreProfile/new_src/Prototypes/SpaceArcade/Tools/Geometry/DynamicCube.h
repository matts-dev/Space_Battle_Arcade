#pragma once
#include <array>
#include "../DataStructures/SATransform.h"
#include <vector>
#include "SimpleShapes.h"
#include "../../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"

namespace SA
{
	enum VertsIndexs
	{
		LBF=0,
		RBF,
		RTF,
		LTF,
		LBN,
		RBN,
		RTN,
		LTN,
	};

	struct DynamicCube
	{
		std::vector<glm::vec4> verts = 
		{
			//x      y      z    
			glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), //LBF
			glm::vec4(0.5f, -0.5f, -0.5f , 1.0f), //RBF
			glm::vec4(0.5f,  0.5f, -0.5f , 1.0f), //RTF
			glm::vec4(-0.5f,  0.5f, -0.5f, 1.0f), //LTF
			glm::vec4(-0.5f, -0.5f, 0.5f , 1.0f), //LBN
			glm::vec4(0.5f, -0.5f, 0.5f  , 1.0f), //RBN
			glm::vec4(0.5f,  0.5f, 0.5f  , 1.0f), //RTN
			glm::vec4(-0.5f,  0.5f, 0.5f , 1.0f)  //LTN
		};
		std::vector<SAT::Shape::EdgePointIndices> edgeIndices = 
		{ 
			//edges on far/near faces
			SAT::Shape::EdgePointIndices{ LBF, RBF},
			SAT::Shape::EdgePointIndices{ LTF, RTF},
			SAT::Shape::EdgePointIndices{ LBN, RBN},
			SAT::Shape::EdgePointIndices{ LTN, RTN},

			//edges on left side
			SAT::Shape::EdgePointIndices{ LBF, LBN},
			SAT::Shape::EdgePointIndices{ LTF, LTN},

			//edges on right side
			SAT::Shape::EdgePointIndices{ RBF, RBN},
			SAT::Shape::EdgePointIndices{ RTF, RTN},
		};
		std::vector<SAT::Shape::FacePointIndices> faceIndices = 
		{

		};
		//struct FacePointIndices
		//{
		//	EdgePointIndices edge1;
		//	EdgePointIndices edge2;
		//};
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dynamic cube that is intended to be modified by user. Implementation for rendering and converting cube to triangles
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class DynamicCubeShapeMesh final : public ShapeMesh
	{
	public:
		void setCube(const DynamicCube& newCube);
		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;
		virtual const std::vector<unsigned int>& getVAOs() override { return vaos; }
	private:
		virtual void onAcquireOpenGLResources() override;
		void doAcquireResources();

		virtual void onReleaseOpenGLResources() override;
		void doReleaseResources();
	private:
		void rebuildVerts();
	private:
		//using TriangleCCW = SAT::DynamicTriangleMeshShape::TriangleProcessor::TriangleCCW;

		std::vector<unsigned int> vaos;
		std::vector<glm::vec3> mappedVerts;
		//std::vector<TriangleCCW> mappedTris;
	private:
		DynamicCube cube;
	};
}