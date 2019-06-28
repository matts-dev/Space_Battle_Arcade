#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include<cstddef> //offsetof macro
//#include "../../../../Shader.h"

namespace SAT
{
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

		template <typename Shader>
		void draw(Shader& shader) const;

		template <typename Shader>
		void drawInstanced(Shader& shader, unsigned int instanceCount) const;

		GLuint getVAO();
		void setInstancedModelMatrixVBO(GLuint modelVBO);
		void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);

	public:
		inline const std::vector<Vertex>&			getVertices() const { return vertices;}
		inline const std::vector<MaterialTexture>&	getTextures() const { return textures;}
		inline const std::vector<unsigned int>&		getIndices () const { return indices; }
	};












	template <typename Shader>
	void LoadedMesh::draw(Shader& shader) const
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


	template <typename Shader>
	void LoadedMesh::drawInstanced(Shader& shader, unsigned int instanceCount) const
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

}
