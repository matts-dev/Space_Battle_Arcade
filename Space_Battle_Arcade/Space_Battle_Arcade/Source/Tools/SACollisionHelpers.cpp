#include "SACollisionHelpers.h"
#include "ModelLoading/SAModel.h"
#include "ModelLoading/SAMesh.h"
#include "ReferenceCode/OpenGL/Algorithms/SeparatingAxisTheorem/SATRenderDebugUtils.h"
#include "Rendering/RenderData.h"
#include "GameFramework/SARenderSystem.h"
#include "GameFramework/SAGameBase.h"
#include "Rendering/SAShader.h"

namespace SA
{


	SAT::DynamicTriangleMeshShape::TriangleProcessor modelToCollisionTriangles(const SA::Model3D& model)
	{
		using TriangleCCW = SAT::DynamicTriangleMeshShape::TriangleProcessor::TriangleCCW;

		const SA::Mesh3D& mesh = model.getMeshes()[0]; //just assume collision models only have a single mesh

		const std::vector<SA::Vertex>& vertices = mesh.getVertices();
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
		SAT::DynamicTriangleMeshShape::TriangleProcessor triProcessor{ dynamicTriangles, treatDotProductSameDeltaThreshold };

		return triProcessor;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ShapeRenderWrapper
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////
	// statics
	////////////////////////////////////////////////////////
	sp<Shader> ShapeRenderWrapper::debugShader = nullptr;
	int32_t ShapeRenderWrapper::instanceCount = 0;

	////////////////////////////////////////////////////////
	// methods
	////////////////////////////////////////////////////////

	ShapeRenderWrapper::ShapeRenderWrapper(const sp<SAT::Shape>& IncollisionShape) 
		: collisionShape(IncollisionShape)
	{
		instanceCount++;
	}

	ShapeRenderWrapper::~ShapeRenderWrapper()
	{
		instanceCount--;
		if (instanceCount == 0)
		{
			debugShader = nullptr;
		}
	}

	void ShapeRenderWrapper::onReleaseGPUResources()
	{
		collisionShapeRenderer = nullptr;
	}

	void ShapeRenderWrapper::onAcquireGPUResources()
	{
		if (!hasAcquiredResources() && collisionShape)
		{
			//#TODO #memory #duplicates should avoid rendering these in real scenarios as each instance of this object gets a copy.
			//need add this to a shared location index by the shape filepath or something. Only using this as a debug tool currently
			//so it isn't critical that this happens now. 
			collisionShapeRenderer = new_sp<SAT::ShapeRender>(*collisionShape);
		}
		if (!debugShader)
		{
			debugShader = new_sp<Shader>(SAT::DebugShapeVertSrc, SAT::DebugShapeFragSrc, false);
		}
	}

	void ShapeRenderWrapper::render(const ShapeRenderOverrides& overrides) const
	{
		Shader* targetShader = bool(overrides.shader) ? overrides.shader : debugShader.get();

		if (hasAcquiredResources() && collisionShape && collisionShapeRenderer && targetShader)
		{
			static GameBase& game = GameBase::get();
			static RenderSystem& renderSystem = game.getRenderSystem();
			
			if (const RenderData* FRD = renderSystem.getFrameRenderData_Read(game.getFrameNumber()))
			{
				targetShader->use();
				glm::mat4 model = xform.getModelMatrix();
				if (overrides.parentXform)
				{
					model = (*overrides.parentXform) * model;
				}

				targetShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				targetShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
				targetShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));

				collisionShapeRenderer->render();
			}
		}
	}
}

