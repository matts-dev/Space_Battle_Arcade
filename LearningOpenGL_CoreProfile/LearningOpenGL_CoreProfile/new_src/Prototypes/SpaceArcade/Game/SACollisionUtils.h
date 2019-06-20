#pragma once
#include <array>
#include <vector>

#include "..\..\..\Algorithms\SpatialHashing\SpatialHashingComponent.h"
#include "..\..\..\Algorithms\SeparatingAxisTheorem\SATComponent.h"

#include "..\GameFramework\SASubsystemBase.h"
#include "..\Tools\DataStructures\SATransform.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"
#include <map>
#include "..\Rendering\SAShader.h"

namespace SA
{
	class WorldEntity;

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Collision information configured for a model; includes shapes for separating axis theorem 
	// and bounding box for spatial hashing
	/////////////////////////////////////////////////////////////////////////////////////////////
	struct ModelCollisionInfo : public RemoveMoves, public RemoveCopies
	{
		std::array<glm::vec4, 8> localAABB;
		std::vector<sp<SAT::Shape>> configuredCollisionShapes;
		std::vector<Transform> configuredCollisionShapeTransforms;
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