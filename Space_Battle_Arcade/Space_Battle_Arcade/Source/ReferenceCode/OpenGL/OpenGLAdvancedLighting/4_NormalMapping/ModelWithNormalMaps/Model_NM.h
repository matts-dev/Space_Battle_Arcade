#pragma once
#include <vector>
#include "LoadedMesh_NM.h"
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <stdexcept>

#include "ReferenceCode/OpenGL/nu_utils.h"
#include "ReferenceCode/OpenGL/Shader.h"

class Model_NM
{
public:

	Model_NM(const char* path);
	~Model_NM();
	void draw(Shader& shader);
	void drawInstanced(Shader& shader, unsigned int instanceCount);

	void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);
private: //members
	std::vector<LoadedMesh_NM> meshes;
	std::string directory;
	std::vector<MaterialTexture> texturesLoaded;

private://methods

	void loadModel_NM(std::string path);

	void processNode(aiNode* node, const aiScene* scene);

	LoadedMesh_NM processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<MaterialTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

