#include "DynamicCube.h"
#include <stdexcept>

namespace SA
{
	void DynamicCubeShapeMesh::render() const
	{
		throw std::runtime_error(__FUNCTION__ " not implemented");
	}

	void DynamicCubeShapeMesh::instanceRender(int instanceCount) const
	{
		throw std::runtime_error(__FUNCTION__ " not implemented");
	}

	void DynamicCubeShapeMesh::onAcquireOpenGLResources()
	{
		doAcquireResources();
	}
	void DynamicCubeShapeMesh::doAcquireResources()
	{
		throw std::runtime_error(__FUNCTION__ " not implemented");
	}

	void DynamicCubeShapeMesh::onReleaseOpenGLResources()
	{
		doReleaseResources();
	}
	void DynamicCubeShapeMesh::doReleaseResources()
	{
		throw std::runtime_error(__FUNCTION__ " not implemented");
	}

	void DynamicCubeShapeMesh::setCube(const DynamicCube& newCube)
	{
		cube = newCube;
		rebuildVerts();
	}

	void DynamicCubeShapeMesh::rebuildVerts()
	{
		//FRONT FACE
		//TriangleCCW front_tri1;
		//TriangleCCW front_tri2;
		//mappedTris.push_back(front_tri1);
		//mappedTris.push_back(front_tri2);

		if (hasAcquiredOpenglResources())
		{
			doReleaseResources();
			doAcquireResources();
		}
	}

}

