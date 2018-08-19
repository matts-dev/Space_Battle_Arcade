#include "Transformable.h"



glm::mat4 Transformable::getLocalTransform()
{
	glm::mat4 transform(1.f);

	//below transformation take transform * newMatrix; ie transform is on the left of the multiply
	transform = glm::translate(transform, position);
	transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
	transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
	transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
	transform = glm::scale(transform, scale);

	return transform;
}

Transformable::Transformable()
{
}


Transformable::~Transformable()
{
}

void Transformable::render(const glm::mat4& projection, const glm::mat4& view, MatrixStack& mstack, Shader& shader)
{
	const glm::mat4 model = mstack.pushMatrix(getLocalTransform());
	renderInternal(projection, view, model, shader);

	//render children with configured matrix stack
	for (Transformable* child : children)
	{
		child->render(projection, view, mstack, shader);
	}

	mstack.popMatrix();
}

void Transformable::setPosition(glm::vec3 newPosition)
{
	position = newPosition;
}

void Transformable::setRotation(glm::vec3 newRotation)
{
	rotation = newRotation;
}

void Transformable::setScale(glm::vec3 newScale)
{
	scale = newScale;
}

void Transformable::addChild(Transformable* newChild)
{
	children.insert(newChild);
}

void Transformable::removeChild(Transformable* currentChild)
{
	children.erase(currentChild);
}
