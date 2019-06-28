#include "SACollisionShapes.h"
#include "..\GameFramework\SALog.h"
#include "..\..\..\Algorithms\SeparatingAxisTheorem\ModelLoader\SATModel.h"
#include "..\..\..\Algorithms\SeparatingAxisTheorem\SATComponent.h"


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
		try { pyramidModel = new_sp<SAT::Model>("GameData/custom_collision_shapes/pyramid_x.obj"); }
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


	sp<SAT::Shape> CollisionShapeFactory::generateShape(ECollisionShape shape) const
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
