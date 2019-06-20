#include "SACollisionUtils.h"
#include "..\Rendering\SAShader.h"
#include "..\..\..\Algorithms\SpatialHashing\SHDebugUtils.h"
#include "..\Rendering\OpenGLHelpers.h"
#include "..\GameFramework\SAWorldEntity.h"

namespace SA
{

	//////////////////////////////////////////////////////////////////////////////////////////////


	sp<SA::ModelCollisionInfo> createUnitCubeCollisionInfo()
	{
		/** axis aligned bounding box(AABB); transform each point to get OBB */
		sp<ModelCollisionInfo> defaultInfo = new_sp<ModelCollisionInfo>();
		defaultInfo->configuredCollisionShapes.push_back(new_sp<SAT::CubeShape>());
		defaultInfo->configuredCollisionShapeTransforms.push_back(Transform{});
		defaultInfo->localAABB[0] = SH::AABB[0];
		defaultInfo->localAABB[1] = SH::AABB[1];
		defaultInfo->localAABB[2] = SH::AABB[2];
		defaultInfo->localAABB[3] = SH::AABB[3];
		defaultInfo->localAABB[4] = SH::AABB[4];
		defaultInfo->localAABB[5] = SH::AABB[5];
		defaultInfo->localAABB[6] = SH::AABB[6];
		defaultInfo->localAABB[7] = SH::AABB[7];
		return defaultInfo;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Spatial hashing debug information
	/////////////////////////////////////////////////////////////////////////////////////////////

	/*static*/ void SpatialHashCellDebugVisualizer::clearCells(SH::SpatialHashGrid<WorldEntity>& grid)
	{
		auto& gridCells = gridNameToCells[&grid];
		gridCells.clear();
	}

	/*static*/ void SpatialHashCellDebugVisualizer::appendCells(SH::SpatialHashGrid<WorldEntity>& grid, SH::HashEntry<WorldEntity>& collisionHandle)
	{
		auto& gridCellLocs = gridNameToCells[&grid];

		static std::vector<std::shared_ptr<const SH::HashCell<WorldEntity>>> cells;
		static std::vector<glm::ivec3> cellLocs;
		static int oneTimeStaticLocalSetup = [&]() -> int {
			cells.reserve(500);
			cellLocs.reserve(500);
			return 0;
		}();

		//clear contents from previous call
		cells.clear();
		cellLocs.clear();

		grid.lookupCellsForEntry(collisionHandle, cells);

		cellLocs.reserve(cells.size());
		for (const std::shared_ptr<const SH::HashCell<WorldEntity>>& cell : cells)
		{
			cellLocs.push_back(cell->location);
		}

		gridCellLocs.insert(std::end(gridCellLocs), std::begin(cellLocs), end(cellLocs));
	}

	/*static*/ void SpatialHashCellDebugVisualizer::render(
		SH::SpatialHashGrid<WorldEntity>& grid,
		const glm::mat4& view,
		const glm::mat4& projection,
		glm::vec3 color /*= glm::vec3(1,1,1)*/)
	{
		static Shader debugLineShader{ SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false };
		auto& gridCells = gridNameToCells[&grid];

		ec(glDepthFunc(GL_ALWAYS));
		SH::drawCells(gridCells, grid.gridCellSize, color, debugLineShader, glm::mat4(1.0f), view, projection);
		ec(glDepthFunc(GL_LESS));

		gridCells.reserve(gridCells.size());
		gridCells.clear();
	}

	std::map<
		SH::SpatialHashGrid<WorldEntity>*,
		std::vector<glm::ivec3>
	> SpatialHashCellDebugVisualizer::gridNameToCells;

}