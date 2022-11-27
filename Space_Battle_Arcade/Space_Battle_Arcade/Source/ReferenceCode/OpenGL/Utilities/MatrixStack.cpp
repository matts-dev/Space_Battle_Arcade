#include "MatrixStack.h"


#include <stdexcept>

MatrixStack::~MatrixStack()
{
}

MatrixStack::MatrixStack()
{
	//start with an identity matrix.
	matStack.push(glm::mat4(1.f));
}

const glm::mat4& MatrixStack::pushMatrix(const glm::mat4& transform)
{
	const glm::mat4& tos = matStack.top();
	matStack.push(tos * transform);
	return matStack.top();
}

void MatrixStack::popMatrix()
{
	matStack.pop();

	if (matStack.size() < 1) throw std::runtime_error("matrix stack underflow: matrix stack lost identity matrix");
}

const glm::mat4& MatrixStack::getCurrentTransform()
{
	return matStack.top();
}
