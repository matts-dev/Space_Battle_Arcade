#include "CubeTexturedMesh.h"



CubeTexturedMesh::CubeTexturedMesh()
{
	//float vertices[] = {
	//	//x      y      z          s     t
	//	-0.5f, -0.5f, -0.5f,      0.0f, 0.0f,
	//	0.5f, -0.5f, -0.5f,      1.0f, 0.0f,
	//	0.5f,  0.5f, -0.5f,      1.0f, 1.0f,
	//	0.5f,  0.5f, -0.5f,      1.0f, 1.0f,
	//	-0.5f,  0.5f, -0.5f,      0.0f, 1.0f,
	//	-0.5f, -0.5f, -0.5f,      0.0f, 0.0f,
	//						    
	//	-0.5f, -0.5f,  0.5f,      0.0f, 0.0f,
	//	0.5f, -0.5f,  0.5f,      1.0f, 0.0f,
	//	0.5f,  0.5f,  0.5f,      1.0f, 1.0f,
	//	0.5f,  0.5f,  0.5f,      1.0f, 1.0f,
	//	-0.5f,  0.5f,  0.5f,      0.0f, 1.0f,
	//	-0.5f, -0.5f,  0.5f,      0.0f, 0.0f,
	//						    
	//	-0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//	-0.5f,  0.5f, -0.5f,      1.0f, 1.0f,
	//	-0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//	-0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//	-0.5f, -0.5f,  0.5f,      0.0f, 0.0f,
	//	-0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//						    
	//	0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//	0.5f,  0.5f, -0.5f,      1.0f, 1.0f,
	//	0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//	0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//	0.5f, -0.5f,  0.5f,      0.0f, 0.0f,
	//	0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//						    
	//	-0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//	0.5f, -0.5f, -0.5f,      1.0f, 1.0f,
	//	0.5f, -0.5f,  0.5f,      1.0f, 0.0f,
	//	0.5f, -0.5f,  0.5f,      1.0f, 0.0f,
	//	-0.5f, -0.5f,  0.5f,      0.0f, 0.0f,
	//	-0.5f, -0.5f, -0.5f,      0.0f, 1.0f,
	//						    
	//	-0.5f,  0.5f, -0.5f,      0.0f, 1.0f,
	//	0.5f,  0.5f, -0.5f,      1.0f, 1.0f,
	//	0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//	0.5f,  0.5f,  0.5f,      1.0f, 0.0f,
	//	-0.5f,  0.5f,  0.5f,      0.0f, 0.0f,
	//	-0.5f,  0.5f, -0.5f,      0.0f, 1.0f 
	//};

	//glGenVertexArrays(1, &vao);
	//glBindVertexArray(vao);

	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
	//glEnableVertexAttribArray(0);

	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
	//glEnableVertexAttribArray(1);

	//glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.
	float vertices[] = {
		//x     y       z      _____normal_xyz___	  s     t
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	 0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,	 0.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,		1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,		1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,		1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,	 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,	 0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,	 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,	 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,	 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,	 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,	 0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,	 1.0f, 0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,	1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,	 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,	1.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,	 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,	 0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,	 0.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,	1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,	1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,	1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,	 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,     0.0f, 1.0f
	};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); //before unbinding any buffers, make sure VAO isn't recording state.
}


CubeTexturedMesh::~CubeTexturedMesh()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

void CubeTexturedMesh::render()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}
