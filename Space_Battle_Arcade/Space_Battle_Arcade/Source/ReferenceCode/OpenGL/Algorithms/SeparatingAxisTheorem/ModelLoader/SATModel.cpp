#include "SATModel.h"
#include "ReferenceCode/OpenGL/nu_utils.h"

namespace SAT
{
	Model::Model(const char* path)
	{
		loadModel(path);
	}

	Model::~Model()
	{

	}


	/* must be called after ctor creates vao*/
	void Model::setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count)
	{
		for (LoadedMesh& mesh : meshes)
		{
			mesh.setInstancedModelMatricesData(modelMatrices, count);
		}
	}

	void Model::loadModel(std::string path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cerr << "Assimp: error importing model: ERROR::" << importer.GetErrorString() << std::endl;
			throw std::runtime_error("failed to load required model");
		}

		directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

	void Model::processNode(aiNode* node, const aiScene* scene)
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

	LoadedMesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<MaterialTexture> textures;

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

			vertices.push_back(vertex);
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

			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());
		}

		return LoadedMesh(vertices, textures, indices);
	}

	std::vector<MaterialTexture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
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
}
