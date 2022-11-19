#pragma once
#include "ReferenceCode/OpenGL/Utilities/Transformable.h"
#include "Mesh.h"


class MeshInstance : public Transformable
{
	Mesh& sourceMesh;

public:
	MeshInstance(Mesh& sourceMesh);
	virtual ~MeshInstance();
	
	virtual void renderInternal(const glm::mat4& projection,
		const glm::mat4& view,
		const glm::mat4& model,
		Shader& shader
	)
	{
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

		sourceMesh.render();
	}
};

