#pragma once
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include "Mesh.h"


class CubeTexturedMesh : public Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;

public:
	CubeTexturedMesh();
	~CubeTexturedMesh();
	void render();

	//special member functions
	CubeTexturedMesh(const CubeTexturedMesh& copy) = delete;
	CubeTexturedMesh(CubeTexturedMesh&& move) = delete;
	CubeTexturedMesh operator=(const CubeTexturedMesh& copy) = delete;
	CubeTexturedMesh operator=(CubeTexturedMesh&& move) = delete;

private:
};

