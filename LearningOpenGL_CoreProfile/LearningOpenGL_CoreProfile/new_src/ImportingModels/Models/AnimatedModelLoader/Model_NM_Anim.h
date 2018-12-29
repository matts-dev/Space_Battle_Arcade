#pragma once
#include <vector>
#include "LoadedMesh_NM_Anim.h"
#include <assimp\scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <stdexcept>

#include "..\..\..\nu_utils.h"
#include "..\..\..\..\Shader.h"

class Model_NM_Anim
{
public:

	Model_NM_Anim(const char* path);
	~Model_NM_Anim();
	void draw(Shader& shader);
	void drawInstanced(Shader& shader, unsigned int instanceCount);

	void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);
private: //members
	std::vector<LoadedMesh_NM_Anim> meshes;
	std::string directory;
	std::vector<MaterialTexture> texturesLoaded;
	std::vector<Bone> allBones;

private://methods

	void loadModel_NM_Anim(std::string path);

	void processNode(aiNode* node, const aiScene* scene);
	void processAnimations(const aiScene* scene);

	LoadedMesh_NM_Anim processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<MaterialTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

};

