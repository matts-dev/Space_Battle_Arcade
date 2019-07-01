#pragma once
#include <array>
#include <vector>
#include <map>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "..\..\..\Algorithms\SpatialHashing\SpatialHashingComponent.h"
#include "..\..\..\Algorithms\SeparatingAxisTheorem\SATComponent.h"

#include "..\GameFramework\SASystemBase.h"
#include "..\Tools\DataStructures\SATransform.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"
#include "..\Rendering\SAShader.h"

namespace SA
{
	enum class ECollisionShape : int;
	class WorldEntity;
	class SpawnConfig;

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Collision information configured for a model; includes shapes for separating axis theorem 
	// and bounding box for spatial hashing
	//
	// This class is designed to be strictly const-correct. Const objects have much access restricted,
	// but enough access to be usable. Non-const objects are essentially modifiable like a struct
	/////////////////////////////////////////////////////////////////////////////////////////////
	class ModelCollisionInfo : public RemoveMoves, public RemoveCopies
	{
	public:
		/** code duplication isn't great, but this is an easy way to provide a const-view 
			of the shape data that is also cache coherent friendly*/
		struct ShapeData
		{
			glm::mat4 localXform;
			sp<SAT::Shape> shape;			//still modifiable with const ShapeData; which is what we want in this case.
			ECollisionShape shapeType;
		};
		struct ConstShapeData
		{
			const glm::mat4 localXform;
			const sp<const SAT::Shape> shape;  //this is why we need a separate stuct, const sp<TYPE> allows nonconst access to TYPE
			const ECollisionShape shapeType;

			ConstShapeData(ShapeData& src) 
				: localXform(src.localXform), shape(src.shape), shapeType(src.shapeType) {}
		};


	public: //take a look at data members, these are accesses that provide struct-like access to non-const objs
		const glm::mat4& getRootXform() const { return rootXform; }
		void setRootXform(const glm::mat4& newXform) { rootXform = newXform; }

		/** Immutable access for const objects. Mutable access for non-const objects;
		set each element directly to prevent array copying */
		const std::array<glm::vec4, 8>& getLocalAABB() const { return localAABB; }
		std::array<glm::vec4, 8>& getLocalAABB() { return localAABB; }

		const glm::mat4& getAABBLocalXform() const { return aabbLocalXform; }
		void setAABBLocalXform(const glm::mat4& newLocalAABBXform) { aabbLocalXform = newLocalAABBXform; }

		const sp<const SAT::Shape>& getOBBShape() const {return obbShape;}
		const sp<SAT::Shape>& getOBBShape(){ return obbShape; }
		void setOBBShape(sp<SAT::Shape> inShape) { obbShape = inShape; }

		/** const version returns an immutable SAT::Shape object
			non-const version is mostly immutable, but the shape object can be manipulated (but not changed to a new shape)
			In all cases vectors are const to prevent resizing outside of the addNewCollisionShape method*/
		const std::vector<ConstShapeData>& getConstShapeData() const { return constShapeData; }
		const std::vector<ShapeData>& getShapeData(){ return shapeData; }
		void addNewCollisionShape(ShapeData& newShape)
		{
			ConstShapeData cShapeData(newShape);
			shapeData.push_back(newShape);
			constShapeData.push_back(cShapeData);
		}

	private:
		/** local transform that moves the model so the origin is correctly positioned for pivoting */
		glm::mat4 rootXform = glm::mat4(1.f);

		/** AABB that has been transformed relative to local model transform; it is used by spatial hashing for cell calculationsl */
		std::array<glm::vec4, 8> localAABB;

		/** OBB pretest optimization; #aabbLocalXform a scales unit cube AABB, then translates to center
			typically column mat usage should be something like: obbMat = worldEntityModalMat * aabbLocalXform; 
			obbShape will need its transform updated with the matrix above*/
		glm::mat4 aabbLocalXform = glm::mat4{ 1.f };
		sp<SAT::Shape> obbShape;

		/** SAT collision shapes and their local transforms; see ShapeData documentation about code duplication
		 Grouping data into structs *may* help with cache coherency*/
		std::vector<ShapeData> shapeData; 
		std::vector<ConstShapeData> constShapeData;

	};

	/** This should be used for quick testing, but proper collision should be configured per entity via an artist; this just returns a configured cube collision*/
	sp<ModelCollisionInfo> createUnitCubeCollisionInfo();

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Spatial hashing debug information
	/////////////////////////////////////////////////////////////////////////////////////////////
	class SpatialHashCellDebugVisualizer
	{
	public:
		static void clearCells(SH::SpatialHashGrid<WorldEntity>& grid);
		static void appendCells(SH::SpatialHashGrid<WorldEntity>& grid, SH::HashEntry<WorldEntity>& collisionHandle);
		static void render(
			SH::SpatialHashGrid<WorldEntity>& grid,
			const glm::mat4& view,
			const glm::mat4& projection, 
			glm::vec3 color = glm::vec3(1, 1, 1));

	private:
		static std::map<
			SH::SpatialHashGrid<WorldEntity>*,
			std::vector<glm::ivec3>
		> gridNameToCells;

		static sp<Shader> debugShader;
	};
}