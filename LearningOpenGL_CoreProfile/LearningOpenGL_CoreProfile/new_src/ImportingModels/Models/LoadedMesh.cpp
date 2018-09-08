#include "LoadedMesh.h"


LoadedMesh::LoadedMesh(std::vector<Vertex> vertices, std::vector<MaterialTexture> textures, std::vector<unsigned int> indices) : vertices(vertices), textures(textures), indices(indices)
{
	setupMesh();
}

LoadedMesh::~LoadedMesh()
{

}

void LoadedMesh::draw(Shader& shader)
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
