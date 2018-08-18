#pragma once

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <stack>

class MatrixStack final
{
public:
	MatrixStack();
	~MatrixStack();
	
	MatrixStack(const MatrixStack& copy)			= delete;
	MatrixStack(MatrixStack&& move)					= delete;
	MatrixStack operator=(const MatrixStack& copy)	= delete;
	MatrixStack operator=(MatrixStack&& move)		= delete;

public: //api
	const glm::mat4& pushMatrix(const glm::mat4& transform);
	const glm::mat4& getCurrentTransform();
	void popMatrix();

private:
	std::stack<glm::mat4> matStack;
};

