#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "MatrixStack.h"
#include "../../Shader.h"


class Transformable
{
private:
	std::vector<Transformable*> children;

private:
	/* This does not consider parent transforms and should not be used publically */
	glm::mat4 getLocalTransform();

protected:
	glm::vec3 position{ 0, 0, 0 };
	glm::vec3 rotation{ 0, 0, 0 };
	glm::vec3 scale{ 1, 1, 1 };

public:
	Transformable();
	virtual ~Transformable();
	
	virtual void render(const glm::mat4& projection, const glm::mat4& view, MatrixStack& mstack, Shader& shader) final;

	void setPosition(glm::vec3 newPosition);
	void setRotation(glm::vec3 newRotation);
	void setScale(glm::vec3 newScale);

protected:
	//abstract function responsible for doing custom rendering of mesh
	virtual void renderInternal(const glm::mat4& projection, 
								const glm::mat4& view,
								const glm::mat4& model,
								Shader& shader) = 0;

};

