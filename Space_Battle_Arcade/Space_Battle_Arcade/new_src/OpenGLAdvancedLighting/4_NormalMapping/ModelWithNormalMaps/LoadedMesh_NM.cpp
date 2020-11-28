#include "LoadedMesh_NM.h"
#include <iostream>


void LoadedMesh_NM::setupMesh()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &VBO_TANGENTS);
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




	//crashes on glBufferData when all data contained in a single VBO; spliting up by using another VBO
	//enable tangent storage
	glBindBuffer(GL_ARRAY_BUFFER, VBO_TANGENTS);
	glBufferData(GL_ARRAY_BUFFER, sizeof(NormalData) * normalData.size(), &normalData[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(NormalData), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(3);

	//enable bitangent
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(NormalData), reinterpret_cast<void*>(offsetof(NormalData, bitangent)));
	glEnableVertexAttribArray(4);
}

LoadedMesh_NM::LoadedMesh_NM(std::vector<Vertex>& vertices, std::vector<MaterialTexture>& textures, std::vector<unsigned int>& indices, std::vector<NormalData>& normalData) : vertices(vertices), textures(textures), indices(indices), normalData(normalData)
{
	setupMesh();
}

LoadedMesh_NM::~LoadedMesh_NM()
{
	//be careful, copy ctor kills buffers silently if these are uncommented (currently happening in instancing tutorial)
	//glDeleteBuffers(1, &VBO);
	//glDeleteBuffers(1, &EAO);
	//glDeleteBuffers(1, &modelVBO);
	//glDeleteVertexArrays(1, &VAO);
}

void LoadedMesh_NM::draw(Shader& shader)
{
	using std::string;

	unsigned int diffuseTextureNumber = 0;
	unsigned int specularTextureNumber = 0;
	unsigned int normalMapTextureNumber = 0;
	unsigned int currentTextureUnit = GL_TEXTURE0;

	for (unsigned int i = 0; i < textures.size(); ++i)
	{
		string uniformName = textures[i].type;
		if (uniformName == "texture_diffuse")
		{
			//naming convention for diffuse is `texture_diffuseN`
			uniformName = string("material.") + uniformName + std::to_string(diffuseTextureNumber);
			++diffuseTextureNumber;
		}
		else if (uniformName == "texture_specular")
		{
			uniformName = string("material.") + uniformName + std::to_string(specularTextureNumber);
			++specularTextureNumber;
		}
		else if (uniformName == "texture_ambient")
		{
			uniformName = string("material.") + uniformName + std::to_string(0);
		}
		else if (uniformName == "texture_normalmap")
		{
			uniformName = string("material.") + uniformName + std::to_string(normalMapTextureNumber);
			++normalMapTextureNumber;
		}
		glActiveTexture(currentTextureUnit);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
		shader.setUniform1i(uniformName.c_str(), currentTextureUnit - GL_TEXTURE0);
		++currentTextureUnit;
	}

	//deactivate other texture units
	//for (unsigned int i = currentTextureUnit; i <= GL_TEXTURE19; ++i)
	//{
	//	glActiveTexture(i);
	//	glBindTexture(GL_TEXTURE_2D, 0); //this just binds to default texture, it isn't reliable for getting a black texture in place. uniforms should be used to control turning textures on and off.
	//}

	//draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void LoadedMesh_NM::drawInstanced(Shader& shader, unsigned int instanceCount) const
{
	using std::string;

	unsigned int diffuseTextureNumber = 0;
	unsigned int specularTextureNumber = 0;
	unsigned int normalMapTextureNumber = 0;
	unsigned int currentTextureUnit = GL_TEXTURE0;

	for (unsigned int i = 0; i < textures.size(); ++i)
	{
		string uniformName = textures[i].type;
		if (uniformName == "texture_diffuse")
		{
			//naming convention for diffuse is `texture_diffuseN`
			uniformName = string("material.") + uniformName + std::to_string(diffuseTextureNumber);
			++diffuseTextureNumber;
		}
		else if (uniformName == "texture_specular")
		{
			uniformName = string("material.") + uniformName + std::to_string(specularTextureNumber);
			++specularTextureNumber;
		}
		else if (uniformName == "texture_ambient")
		{
			uniformName = string("material.") + uniformName + std::to_string(0);
		}
		else if (uniformName == "texture_normalmap")
		{
			uniformName = string("material.") + uniformName + std::to_string(normalMapTextureNumber);
			++normalMapTextureNumber;
		}
		glActiveTexture(currentTextureUnit);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
		shader.setUniform1i(uniformName.c_str(), currentTextureUnit - GL_TEXTURE0);
		++currentTextureUnit;
	}

	//draw mesh
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
	glBindVertexArray(0);
}

/** This really should be a private function, making visible for instancing tutorial */
GLuint LoadedMesh_NM::getVAO()
{
	return VAO;
}

void LoadedMesh_NM::setInstancedModelMatrixVBO(GLuint modelVBO)
{
	this->modelVBO = modelVBO;
}

void LoadedMesh_NM::setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count)
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

