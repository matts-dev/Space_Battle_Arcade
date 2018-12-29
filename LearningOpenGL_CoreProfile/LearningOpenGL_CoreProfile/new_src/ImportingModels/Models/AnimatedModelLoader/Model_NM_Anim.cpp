#include "Model_NM_Anim.h"
#include <map>

Model_NM_Anim::Model_NM_Anim(const char* path)
{
	loadModel_NM_Anim(path);
}

Model_NM_Anim::~Model_NM_Anim()
{

}

void Model_NM_Anim::draw(Shader& shader)
{
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		meshes[i].draw(shader);
	}
}

void Model_NM_Anim::drawInstanced(Shader& shader, unsigned int instanceCount)
{
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		meshes[i].drawInstanced(shader, instanceCount);
	}
}


/* must be called after ctor creates vao*/
void Model_NM_Anim::setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count)
{
	for (LoadedMesh_NM_Anim& mesh : meshes)
	{
		mesh.setInstancedModelMatricesData(modelMatrices, count);
	}
}

void Model_NM_Anim::loadModel_NM_Anim(std::string path)
{
	Assimp::Importer importer; //dtor should clean up scene object
	//--------------------------------------------------------------------------------------------
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	//--------------------------------------------------------------------------------------------

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "Assimp: error importing model: ERROR::" << importer.GetErrorString() << std::endl;
		throw std::runtime_error("failed to load required model");
	}

	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
	processAnimations(scene);
}

void Model_NM_Anim::processNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene);
	}
}

void Model_NM_Anim::processAnimations(const aiScene* scene)
{
	//if(scene->HasMeshes() && scene->HasAnimations())
	//{
	//	for (int meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
	//	{
	//		aiMesh* mesh = scene->mMeshes[meshIdx];
	//		if (mesh->HasBones())
	//		{
	//			
	//		}
	//	}

	//	for (int animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx)
	//	{
	//		
	//	}
	//}
}

LoadedMesh_NM_Anim Model_NM_Anim::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<MaterialTexture> textures;
	std::vector<NormalData> normalData;
	std::vector<VertexBoneData> vertexBoneData;

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;

		//check if model has texture coordinates
		if (mesh->mTextureCoords[0])
		{
			vertex.textureCoords.x = mesh->mTextureCoords[0][i].x;
			vertex.textureCoords.y = mesh->mTextureCoords[0][i].y;

		}
		else
		{
			vertex.textureCoords = { 0.f, 0.f };
		}

		NormalData nmData;
		nmData.tangent.x = mesh->mTangents[i].x;
		nmData.tangent.y = mesh->mTangents[i].y;
		nmData.tangent.z = mesh->mTangents[i].z;
		nmData.tangent = glm::normalize(nmData.tangent);

		nmData.bitangent.x = mesh->mBitangents[i].x;
		nmData.bitangent.y = mesh->mBitangents[i].y;
		nmData.bitangent.z = mesh->mBitangents[i].z;
		nmData.bitangent = glm::normalize(nmData.bitangent);

		vertices.push_back(vertex);
		normalData.push_back(nmData);

		//glm::vec3 calcNormal = glm::normalize(glm::cross(nmData.tangent, nmData.bitangent));
		//float normalMatch = glm::dot(calcNormal, vertex.normal);
		//std::cout << normalMatch << std::endl;
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		//loader forces faces to be triangles, so these indices should be valid for drawing triangles
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<MaterialTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		std::vector<MaterialTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		std::vector<MaterialTexture> ambientMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ambient");
		std::vector<MaterialTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normalmap"); //aiTextureType seems to load normal maps from aiTextureType_HEIGHT, rather than aiTextureType_NORMAL

		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	}

	vertexBoneData.resize(vertices.size()); //default construct all bone weights to 0

	std::map<std::string, Bone> nameToBoneMap;
	if (mesh->HasBones())
	{
		for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
		{
			aiBone* bone = mesh->mBones[boneIdx];

			//cache bone information
			Bone cacheBone;
			cacheBone.name = bone->mName.C_Str();
			cacheBone.meshToBoneTransform = bone->mOffsetMatrix;
			cacheBone.weights.resize(0);
			cacheBone.weights.reserve(bone->mNumWeights);
			cacheBone.uniqueID = static_cast<int>(allBones.size());

			for (unsigned int wgt = 0; wgt < bone->mNumWeights; ++wgt)
			{
				cacheBone.weights.push_back(bone->mWeights[wgt]);

				unsigned int vertId = bone->mWeights[wgt].mVertexId;
				int curBone = vertexBoneData[vertId].currentBone;

				assert(curBone != MAX_NUM_BONES); //assert if we're about to go out of bounds

				//to vertex data, add an influencing bone and its weight; these will be used to lookup into bone transforms array
				vertexBoneData[vertId].boneWeights[curBone] = bone->mWeights[wgt].mWeight;
				vertexBoneData[vertId].boneIds[curBone] = cacheBone.uniqueID;
			}

			allBones.push_back(cacheBone);
			if (nameToBoneMap.find(cacheBone.name) == nameToBoneMap.end())
				nameToBoneMap[cacheBone.name] = cacheBone;
			else
				std::cerr << "duplicate bone " << cacheBone.name << " found when loading mesh" << std::endl;
		}
	}

	return LoadedMesh_NM_Anim(vertices, textures, indices, normalData, vertexBoneData, nameToBoneMap);
}

std::vector<MaterialTexture> Model_NM_Anim::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<MaterialTexture> textures;

	unsigned int textureCount = mat->GetTextureCount(type);
	for (unsigned int i = 0; i < textureCount; i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		std::string relativePath = std::string(str.C_Str());

		std::string filepath = directory + std::string("/") + relativePath;

		bool bShouldSkip = false;
		for (unsigned int j = 0; j < texturesLoaded.size(); ++j)
		{
			if (texturesLoaded[i].path == relativePath)
			{
				//already loaded this texture, just the cached texture information
				textures.push_back(texturesLoaded[j]);
				bShouldSkip = true;
				break;
			}
		}

		if (!bShouldSkip)
		{
			MaterialTexture texture;
			texture.id = textureLoader(filepath.c_str());
			texture.type = typeName;
			texture.path = relativePath;
			textures.push_back(texture);

			//cache for later texture loads
			texturesLoaded.push_back(texture);
		}
	}
	return textures;
}
