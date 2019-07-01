#include "SACollisionUtils.h"
#include "..\Rendering\SAShader.h"
#include "..\..\..\Algorithms\SpatialHashing\SHDebugUtils.h"
#include "..\Rendering\OpenGLHelpers.h"
#include "..\GameFramework\SAWorldEntity.h"
#include "..\Tools\ModelLoading\SAModel.h"
#include "SASpawnConfig.h"

namespace SA
{

	//////////////////////////////////////////////////////////////////////////////////////////////


	sp<SA::ModelCollisionInfo> createUnitCubeCollisionInfo()
	{
		/** axis aligned bounding box(AABB); transform each point to get OBB */
		sp<ModelCollisionInfo> defaultInfo = new_sp<ModelCollisionInfo>();

		ModelCollisionInfo::ShapeData shapeData;
		shapeData.shapeType = ECollisionShape::CUBE;
		shapeData.shape = new_sp<SAT::CubeShape>();
		shapeData.localXform = glm::mat4{1.f};
		defaultInfo->addNewCollisionShape(shapeData);

		std::array<glm::vec4, 8>& defaultInfoAABB = defaultInfo->getLocalAABB();
		defaultInfoAABB[0] = SH::AABB[0];
		defaultInfoAABB[1] = SH::AABB[1];
		defaultInfoAABB[2] = SH::AABB[2];
		defaultInfoAABB[3] = SH::AABB[3];
		defaultInfoAABB[4] = SH::AABB[4];
		defaultInfoAABB[5] = SH::AABB[5];
		defaultInfoAABB[6] = SH::AABB[6];
		defaultInfoAABB[7] = SH::AABB[7];

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