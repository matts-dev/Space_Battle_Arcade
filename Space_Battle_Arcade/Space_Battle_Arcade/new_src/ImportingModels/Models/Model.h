#pragma once
#include <vector>
#include "LoadedMesh.h"
#include <assimp\scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <stdexcept>
#include "..\..\nu_utils.h"

class Model
{
public:

	Model(const char* path);
	~Model();
	void draw(Shader& shader);
	void drawInstanced(Shader& shader, unsigned int instanceCount);

	void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);
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

