#include "ReferenceCode/OpenGL/Utilities/Transformable.h"



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

glm::mat4 Transformable::getTransform()
{
	MatrixStack mstack;
	getRootToLeafTransform(mstack);
	//return mstack.getCurrentTransform();

	throw std::runtime_error("this feature needs testing before use");
}

void Transformable::getRootToLeafTransform(MatrixStack& mstack)
{
	if (parent != nullptr)
	{
		getRootToLeafTransform(mstack);
	}
	mstack.pushMatrix(getLocalTransform());

	throw std::runtime_error("this feature needs testing before use");
}

Transformable::Transformable()
{
}


Transformable::~Transformable()
{
	for (Transformable* child : children)
	{
		//WARNING: becareful not to modify the container with iternal functions while iterating over it.
		child->parent = nullptr;
	}
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
	newChild->parent = this;
}

void Transformable::removeChild(Transformable* currentChild)
{
	children.erase(currentChild);
	currentChild->parent = nullptr;
}

glm::vec3 Transformable::getPosition()
{
	if (parent == nullptr)
	{
		return position;
	}
	else
	{
		glm::mat4 transform = getTransform();
		//return glm::vec3(transform * glm::vec4(position, 1.f)); 
		throw std::runtime_error("this feature needs testing before use");
	}
}
