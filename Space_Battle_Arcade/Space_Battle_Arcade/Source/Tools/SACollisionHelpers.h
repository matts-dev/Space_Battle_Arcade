#pragma once

#include <cstdint>

#include "ReferenceCode/OpenGL/Algorithms/SeparatingAxisTheorem/SATComponent.h"

#include "Tools/DataStructures/SATransform.h"
#include "Tools/RemoveSpecialMemberFunctionUtils.h"
#include "Rendering/SAGPUResource.h"

//#TODO_minor rename this file to be ShapeRenderWrapper and move other functionality to SACollisionUtils
namespace SAT
{
	class ShapeRender;
}

namespace SA
{
	class Model3D;
	class Shader;


	SAT::DynamicTriangleMeshShape::TriangleProcessor modelToCollisionTriangles(const Model3D& model);

	/**
	This can be used to debug the rendered model shape.
	It has a external dependency on SAT, so should be used sparingly
	*/
	class ShapeRenderWrapper : public GPUResource, public RemoveCopies
	{
	public:
		ShapeRenderWrapper(const sp<SAT::Shape>& IncollisionShape);
		~ShapeRenderWrapper();

		struct RenderOverrides
		{
			Shader* shader = nullptr;
			glm::mat4* parentXform = nullptr;
		};
		virtual void render(const RenderOverrides& overrides = {}) const;
		void setXform(const Transform& xform) { this->xform = xform; }
	private:
		virtual void onReleaseGPUResources() override;
		virtual void onAcquireGPUResources() override;
	private:
		static sp<Shader> debugShader;
		static int32_t instanceCount;
	private:
		sp<SAT::Shape> collisionShape;
		sp<SAT::ShapeRender> collisionShapeRenderer;
		Transform xform;
	};
}
