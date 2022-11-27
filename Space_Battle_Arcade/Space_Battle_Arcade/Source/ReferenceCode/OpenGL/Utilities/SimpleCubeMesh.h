#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include "Mesh.h"


class SimpleCubeMesh : public Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;

public:
	SimpleCubeMesh();
	~SimpleCubeMesh();
	void render();

	//special member functions
	SimpleCubeMesh(const SimpleCubeMesh& copy) = delete;
	SimpleCubeMesh(SimpleCubeMesh&& move) = delete;
	SimpleCubeMesh operator=(const SimpleCubeMesh& copy) = delete;
	SimpleCubeMesh operator=(SimpleCubeMesh&& move) = delete;

private:
};

