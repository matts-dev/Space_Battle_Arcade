#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include<cstddef> //offsetof macro
#include "../../../Shader.h"



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
	void setupMesh()
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

public:
	LoadedMesh(std::vector<Vertex> vertices,
				std::vector<MaterialTexture> textures,
				std::vector<unsigned int> indices);
	~LoadedMesh();

	void draw(Shader& shader)
	{
		using std::string;

		unsigned int diffuseTextureNumber = 0;
		unsigned int specularTextureNumber = 0;
		unsigned int currentTextureUnit = GL_TEXTURE0;

		for (unsigned int i = 0; i < textures.size(); ++i)
		{
			string uniformName = textures[i].type;
			if (uniformName == "texture_diffuse")
			{
				//naming convention for diffuse is `texture_diffuseN`
				uniformName = string("material.") + uniformName + std::to_string(currentTextureUnit);
				++diffuseTextureNumber;
			}
			else if (uniformName == "texture_specular")
			{
				uniformName = string("material.") + uniformName + std::to_string(currentTextureUnit);
				++specularTextureNumber;
			}
			glActiveTexture(currentTextureUnit);
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
			shader.setUniform1i(uniformName.c_str(), currentTextureUnit);
			++currentTextureUnit;
		}

		//draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size() , GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
};

