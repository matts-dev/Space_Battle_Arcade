#include "SATLoadedMesh.h"
#include <iostream>

namespace SAT
{
	void LoadedMesh::setupMesh()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EAO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

		//enable vertex data 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		//enable normal data
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
		glEnableVertexAttribArray(1);

		//enable texture coordinate data
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, textureCoords)));
		glEnableVertexAttribArray(2);
	}

	LoadedMesh::LoadedMesh(std::vector<Vertex> vertices, std::vector<MaterialTexture> textures, std::vector<unsigned int> indices) : vertices(vertices), textures(textures), indices(indices)
	{
		setupMesh();
	}

	LoadedMesh::~LoadedMesh()
	{
		//be careful, copy ctor kills buffers silently if these are uncommented (currently happening in instancing tutorial)
		//glDeleteBuffers(1, &VBO);
		//glDeleteBuffers(1, &EAO);
		//glDeleteBuffers(1, &modelVBO);
		//glDeleteVertexArrays(1, &VAO);
	}

	/** This really should be a private function, making visible for instancing tutorial */
	GLuint LoadedMesh::getVAO()
	{
		return VAO;
	}

	void LoadedMesh::setInstancedModelMatrixVBO(GLuint modelVBO)
	{
		this->modelVBO = modelVBO;
	}

	void LoadedMesh::setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count)
	{
		glBindVertexArray(VAO);

		if (modelVBO == 0)
		{
			glGenBuffers(1, &modelVBO);
			glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * count, &modelMatrices[0], GL_STATIC_DRAW);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(0));
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(sizeof(glm::vec4)));
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(2 * sizeof(glm::vec4)));
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(3 * sizeof(glm::vec4)));
			glEnableVertexAttribArray(3);
			glEnableVertexAttribArray(4);
			glEnableVertexAttribArray(5);
			glEnableVertexAttribArray(6);
			glVertexAttribDivisor(3, 1);
			glVertexAttribDivisor(4, 1);
			glVertexAttribDivisor(5, 1);
			glVertexAttribDivisor(6, 1);
		}
		else
		{
			std::cerr << "instanced model matrices VBO already exists" << std::endl;
		}

		glBindVertexArray(0);
	}

}
