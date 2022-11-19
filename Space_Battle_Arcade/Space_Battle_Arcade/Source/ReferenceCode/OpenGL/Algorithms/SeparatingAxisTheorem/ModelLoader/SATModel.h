#pragma once
#include <vector>
#include "SATLoadedMesh.h"
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <stdexcept>
//#include "ReferenceCode/OpenGL/nu_utils.h"

namespace SAT
{
	class Model
	{
	public:

		Model(const char* path);
		~Model();

		template <typename Shader>
		void draw(Shader& shader) const;

		template <typename Shader>
		void drawInstanced(Shader& shader, unsigned int instanceCount);

		void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);

		inline const std::vector<LoadedMesh>& getMeshes() const { return meshes; }

	private: //members
		std::vector<LoadedMesh> meshes;
		std::string directory;
		std::vector<MaterialTexture> texturesLoaded;

	private://methods

		void loadModel(std::string path);
		void processNode(aiNode* node, const aiScene* scene);
		LoadedMesh processMesh(aiMesh* mesh, const aiScene* scene);
		std::vector<MaterialTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	};











	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Template Bodies
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename Shader>
	void Model::draw(Shader& shader) const
	{
		for (unsigned int i = 0; i < meshes.size(); ++i)
		{
			meshes[i].draw(shader);
		}
	}

	template <typename Shader>
	void Model::drawInstanced(Shader& shader, unsigned int instanceCount)
	{
		for (unsigned int i = 0; i < meshes.size(); ++i)
		{
			meshes[i].drawInstanced(shader, instanceCount);
		}
	}
}
