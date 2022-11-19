#include "SACollisionUtils.h"

#include "Game/GameSystems/SAModSystem.h"
#include "Game/SACollisionDebugRenderer.h"
#include "Game/SpaceArcade.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SAWorldEntity.h"
#include "ReferenceCode/OpenGL/Algorithms/SeparatingAxisTheorem/ModelLoader/SATModel.h"
#include "ReferenceCode/OpenGL/Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SHDebugUtils.h"
#include "Rendering/OpenGLHelpers.h"
#include "Rendering/SAShader.h"
#include "GameFramework/SAAssetSystem.h"
#include "Tools/ModelLoading/SAModel.h"
#include "Tools/SACollisionHelpers.h"





////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data local to this cpp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using TriangleCCW = SAT::DynamicTriangleMeshShape::TriangleProcessor::TriangleCCW;
using TriangleProcessor = SAT::DynamicTriangleMeshShape::TriangleProcessor;

namespace
{
	SA::sp<SAT::Model> uvsphereModel;
	SA::sp<SAT::Model> icosphereModel;
	SA::sp<SAT::Model> wedgeModel;
	SA::sp<SAT::Model> pyramidModel;
	SA::sp<SAT::Model> nullModel = nullptr;

	//there's no need to have these in header file; implementation can be done here since they're essentially singletons
	SA::sp<TriangleProcessor> pyramidTriProc = nullptr;
	SA::sp<TriangleProcessor> wedgeTriProc = nullptr;
	SA::sp<TriangleProcessor> icosphereTriProc = nullptr;
	SA::sp<TriangleProcessor> uvsphereTriProc = nullptr;
}














namespace SA
{
	sp<SAT::Shape> tryLoadModelShape(const char* fullFilePath)
	{
		sp<Model3D> newModel = nullptr;
		sp<SAT::Shape> modelCollision = nullptr;
		try
		{
			AssetSystem& assetSystem = GameBase::get().getAssetSystem();
			newModel = assetSystem.loadModel(fullFilePath);
			if (newModel)
			{
				TriangleProcessor processedModel = modelToCollisionTriangles(*newModel); //#TODO_minor this function perhaps should exist in this file
				modelCollision = new_sp<SAT::DynamicTriangleMeshShape>(processedModel);
				return modelCollision; //early out so failure log isn't printed at end of this function.
			}
		}
		catch (...)
		{
			log(__FUNCTION__, LogLevel::LOG, "Failed to load collision model");
		}
		return modelCollision;
	}

	sp<SAT::Shape> tryLoadModelShapeModRelative(const char* modRelativeFilePath)
	{
		//try load even if a file is already in the map. This way we can do file refreshes while running.
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod)
		{
			std::string fullRelativeAssetPath = activeMod->getModDirectoryPath() + modRelativeFilePath;
			return tryLoadModelShape(fullRelativeAssetPath.c_str());
		}

		log(__FUNCTION__, LogLevel::LOG, "Failed to load collision model");
		return sp<SAT::Shape>{nullptr};
	}

	//////////////////////////////////////////////////////////////////////////////////////////////


	sp<SA::CollisionData> createUnitCubeCollisionData()
	{
		/** axis aligned bounding box(AABB); transform each point to get OBB */
		sp<CollisionData> defaultInfo = new_sp<CollisionData>();

		CollisionData::ShapeData shapeData;
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
		//static Shader debugLineShader{ SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false };
		static sp<Shader> debugLineShader = new_sp<Shader>(SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false); //this whole system shoudl be refactored to use debug line renderering.
		auto& gridCells = gridNameToCells[&grid];

		ec(glDepthFunc(GL_ALWAYS));
		SH::drawCells(gridCells, grid.gridCellSize, color, *debugLineShader, glm::mat4(1.0f), view, projection);
		ec(glDepthFunc(GL_LESS));

		gridCells.reserve(gridCells.size());
	}

	std::map<
		SH::SpatialHashGrid<WorldEntity>*,
		std::vector<glm::ivec3>
	> SpatialHashCellDebugVisualizer::gridNameToCells;

	CollisionData::CollisionData()
	{
		setOBBShape(new_sp<SAT::CubeShape>()); //would be nice if we didn't need to heap allocate every time, but each shape has a unique transform
	}

#if SA_RENDER_DEBUG_INFO
	void CollisionData::debugRender(const glm::mat4& modelMat, const glm::mat4& view, const glm::mat4& projection) const
	{
		static sp<CollisionDebugRenderer> collisionDebugRenderer = new_sp<CollisionDebugRenderer>();
		
		if (CollisionDebugRenderer::bRenderCollisionOBB_ui)
		{
			const glm::mat4& aabbLocalXform = getAABBLocalXform();
			collisionDebugRenderer->renderOBB(modelMat, aabbLocalXform, view, projection,
				glm::vec3(0, 0, 1), GL_LINE, GL_FILL);
		}

		if (CollisionDebugRenderer::bRenderCollisionShapes_ui)
		{
			using ConstShapeData = CollisionData::ConstShapeData;
			for (const ConstShapeData shapeData : getConstShapeData())
			{
				collisionDebugRenderer->renderShape(
					shapeData.shapeType,
					modelMat * shapeData.localXform,
					view, projection, glm::vec3(1, 0, 0), GL_LINE, GL_FILL
				);
			}
		}
	}
#endif //SA_RENDER_DEBUG_INFO

	void CollisionData::setAABBtoModelBounds(const Model3D& model, const std::optional<glm::mat4>& staticRootModelOffsetMatrix)
	{
		using namespace glm;

		CollisionShapeFactory& shapeFactory = SpaceArcade::get().getCollisionShapeFactoryRef();

		std::tuple<vec3, vec3> aabbRange = model.getAABB();
		vec3 aabbSize = std::get<1>(aabbRange) - std::get<0>(aabbRange); //max - min

		//correct for model center mis-alignments; this should be cached in game so it isn't calculated each frame
		vec3 aabbCenterPnt = std::get</*min*/0>(aabbRange) + (0.5f * aabbSize);

		//we can now use aabbCenter as a translation vector for the aabb!
		mat4 aabbModel = glm::translate(staticRootModelOffsetMatrix.value_or(glm::mat4(1.f)), aabbCenterPnt);
		aabbModel = glm::scale(aabbModel, aabbSize);
		std::array<glm::vec4, 8>& collisionLocalAABB = getLocalAABB();
		collisionLocalAABB[0] = aabbModel * SH::AABB[0];
		collisionLocalAABB[1] = aabbModel * SH::AABB[1];
		collisionLocalAABB[2] = aabbModel * SH::AABB[2];
		collisionLocalAABB[3] = aabbModel * SH::AABB[3];
		collisionLocalAABB[4] = aabbModel * SH::AABB[4];
		collisionLocalAABB[5] = aabbModel * SH::AABB[5];
		collisionLocalAABB[6] = aabbModel * SH::AABB[6];
		collisionLocalAABB[7] = aabbModel * SH::AABB[7];

		setAABBLocalXform(aabbModel);
		setOBBShape(shapeFactory.generateShape(ECollisionShape::CUBE));
	}

	void CollisionData::updateToNewWorldTransform(glm::mat4 worldXform)
	{
		for (const CollisionData::ShapeData& shapeData : shapeData)
		{
			shapeData.shape->updateTransform(worldXform * shapeData.localXform);
		}

		obbShape->updateTransform(worldXform * aabbLocalXform);
		
		worldOBB[0] = worldXform * localAABB[0];
		worldOBB[1] = worldXform * localAABB[1];
		worldOBB[2] = worldXform * localAABB[2];
		worldOBB[3] = worldXform * localAABB[3];
		worldOBB[4] = worldXform * localAABB[4];
		worldOBB[5] = worldXform * localAABB[5];
		worldOBB[6] = worldXform * localAABB[6];
		worldOBB[7] = worldXform * localAABB[7];

	}

}



















































namespace SA
{
	const char* const shapeToStr(ECollisionShape value)
	{
		switch (value)
		{
			case ECollisionShape::CUBE: return "Cube";
			case ECollisionShape::POLYCAPSULE: return "PolyCapsule";
			case ECollisionShape::WEDGE: return "Wedge";
			case ECollisionShape::PYRAMID: return "Pyramid";
			case ECollisionShape::ICOSPHERE: return "IcoSphere";
			case ECollisionShape::UVSPHERE: return "UVSphere";
			case ECollisionShape::MODEL: return "model";
			default: return "invalid";
		}
	}


	CollisionShapeFactory::CollisionShapeFactory()
	{
		if (::pyramidTriProc || ::wedgeTriProc || ::icosphereTriProc || ::uvsphereTriProc)
		{
			return;
		}

		//load models
		//try { pyramidModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/pyramid_x.obj"); }//this doesn't scale well
		try { pyramidModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/pyramid_rotated.obj"); } //rotated pyramid lets you scale on corners
		catch (std::runtime_error&) { log("CollisionShapeContainer", LogLevel::LOG_ERROR, "Failed to load pyramid model"); }

		try { wedgeModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/cockpitshape.obj"); }
		catch (std::runtime_error&) { log("CollisionShapeContainer", LogLevel::LOG_ERROR, "Failed to load wedge/cockpit model"); }

		try { icosphereModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/icosphere.obj"); }
		catch (std::runtime_error&) { log("CollisionShapeContainer", LogLevel::LOG_ERROR, "Failed to load icosphere model"); }

		try { uvsphereModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/uvsphere.obj"); }
		catch (std::runtime_error&) { log("CollisionShapeContainer", LogLevel::LOG_ERROR, "Failed to load uvsphere model"); }

		//cache the expensive part of converting models to collision shapes
		//this lambda does the work of creating a tri processor, which is passed to a dynamic collision shape ctor
		auto getTriProcessor = [](sp<SAT::Model>& model) -> sp<TriangleProcessor>
		{
			if (model)
			{
				const std::vector<SAT::LoadedMesh>& meshes = model->getMeshes();
				if (meshes.size() > 0)
				{
					const SAT::LoadedMesh& mesh = meshes[0]; //just assume collision models first mesh is the appropriate
					const std::vector<SAT::Vertex>& vertices = mesh.getVertices();
					const std::vector<unsigned int>& indices = mesh.getIndices();

					std::vector<TriangleCCW> dynamicTriangles;

					//loop a triangle at a time
					assert(indices.size() % 3 == 0); //ensure loop will not go out of bounds
					for (unsigned int idx = 0; idx < indices.size(); idx += 3)
					{
						TriangleCCW tri;
						tri.pntA = glm::vec4(vertices[indices[idx]].position, 1.0f);
						tri.pntB = glm::vec4(vertices[indices[idx + 1]].position, 1.0f);
						tri.pntC = glm::vec4(vertices[indices[idx + 2]].position, 1.0f);
						dynamicTriangles.push_back(tri);
					}

					float treatDotProductSameDeltaThreshold = 0.001f;
					return new_sp<TriangleProcessor>(dynamicTriangles, treatDotProductSameDeltaThreshold);
				}
			}
			return nullptr;
		};

		::pyramidTriProc = getTriProcessor(pyramidModel);
		::wedgeTriProc = getTriProcessor(wedgeModel);
		::icosphereTriProc = getTriProcessor(icosphereModel);
		::uvsphereTriProc = getTriProcessor(uvsphereModel);

	}


	sp<SAT::Shape> CollisionShapeFactory::generateShape(ECollisionShape shape, const std::string& optionalFilePath) const
	{
		switch (shape)
		{
			case ECollisionShape::CUBE:
				{
					return new_sp<SAT::CubeShape>();
				}
			case ECollisionShape::POLYCAPSULE:
				{
					return new_sp<SAT::PolygonCapsuleShape>();
				}
			case ECollisionShape::WEDGE:
				{
					if (wedgeTriProc)
					{
						return new_sp<SAT::DynamicTriangleMeshShape>(*wedgeTriProc);
					}
					return nullptr;
				}
			case ECollisionShape::PYRAMID:
				{
					if (pyramidTriProc)
					{
						return new_sp<SAT::DynamicTriangleMeshShape>(*pyramidTriProc);
					}
					return nullptr;
				}
			case ECollisionShape::ICOSPHERE:
				{
					if (icosphereTriProc)
					{
						return new_sp<SAT::DynamicTriangleMeshShape>(*icosphereTriProc);
					}
					return nullptr;

				}
			case ECollisionShape::UVSPHERE:
				{
					if (uvsphereTriProc)
					{
						return new_sp<SAT::DynamicTriangleMeshShape>(*uvsphereTriProc);
					}
					return nullptr;
				}
			case ECollisionShape::MODEL:
				{
					return tryLoadModelShapeModRelative(optionalFilePath.c_str());
				}
			default:
				{
					return nullptr;
				}
		}
	}

	sp<const SAT::Model> CollisionShapeFactory::getModelForShape(ECollisionShape shape) const
	{
		switch (shape)
		{
			case ECollisionShape::WEDGE: return ::wedgeModel;
			case ECollisionShape::PYRAMID: return ::pyramidModel;
			case ECollisionShape::ICOSPHERE: return ::icosphereModel;
			case ECollisionShape::UVSPHERE: return ::uvsphereModel;
			case ECollisionShape::CUBE:
			case ECollisionShape::POLYCAPSULE:
			default:
				{
					return ::nullModel;
				}
		}
	}
}
