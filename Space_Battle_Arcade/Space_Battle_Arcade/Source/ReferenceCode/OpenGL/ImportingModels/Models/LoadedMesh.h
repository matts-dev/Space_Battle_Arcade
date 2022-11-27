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

};

struct MaterialTexture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class LoadedMesh
{
private:
	std::vector<Vertex> vertices;
	std::vector<MaterialTexture> textures;
	std::vector<unsigned int> indices;

	GLuint VAO, VBO, EAO;
	GLuint modelVBO = 0;
	void setupMesh();

public:
	LoadedMesh(std::vector<Vertex> vertices,
				std::vector<MaterialTexture> textures,
				std::vector<unsigned int> indices);
	~LoadedMesh();

	void draw(Shader& shader);
	void drawInstanced(Shader& shader, unsigned int instanceCount) const;
	GLuint getVAO();
	void setInstancedModelMatrixVBO(GLuint modelVBO);
	void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);
};

