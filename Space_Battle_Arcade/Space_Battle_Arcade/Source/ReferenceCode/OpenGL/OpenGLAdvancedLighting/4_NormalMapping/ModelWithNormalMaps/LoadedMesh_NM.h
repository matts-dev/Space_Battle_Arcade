#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include<cstddef> //offsetof macro
#include "ReferenceCode/OpenGL/Shader.h"


struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoords;
	//putting tangent/bitangent here causes crashes on glBufferData, probably because data too large
};

struct NormalData
{
	glm::vec3 tangent;
	glm::vec3 bitangent;
};

struct MaterialTexture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class LoadedMesh_NM
{
private:
	std::vector<Vertex> vertices;
	std::vector<MaterialTexture> textures;
	std::vector<unsigned int> indices;
	std::vector<NormalData> normalData;

	GLuint VAO, VBO, VBO_TANGENTS, EAO;
	GLuint modelVBO = 0;
	void setupMesh();

public:
	LoadedMesh_NM(std::vector<Vertex>& vertices,
		std::vector<MaterialTexture>& textures,
		std::vector<unsigned int>& indices,
		std::vector<NormalData>& normalData);
	~LoadedMesh_NM();

	void draw(Shader& shader);
	void drawInstanced(Shader& shader, unsigned int instanceCount) const;
	GLuint getVAO();
	void setInstancedModelMatrixVBO(GLuint modelVBO);
	void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);
};

