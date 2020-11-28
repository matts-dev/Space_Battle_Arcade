#include "SAModel.h"
#include <map>
#include <chrono>
#include <cstdint>
#include "../SAUtilities.h"
#include "../../Rendering/OpenGLHelpers.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SAGameBase.h"


namespace SA
{

	template<typename T>
	T clampT(T val, T min, T max)
	{
		val = val < min ? min : val;
		val = val > max ? max : val;
		return val;
	}

	Model3D::Model3D(const char* path)
	{
		cachedAABB = std::make_tuple(glm::vec3{ 0,0,0 }, glm::vec3{ 0,0,0 });
		loadModel(path);
	}

	Model3D::~Model3D()
	{
		releaseGPUData();
	}

	void Model3D::releaseGPUData()
	{
		if (!bGPUReleased)
		{
			bGPUReleased = true;
			for (Mesh3D& mesh : meshes)
			{
				mesh.releaseGPUData();
			}
			for (MaterialTexture& mat : texturesLoaded)
			{
				ec(glDeleteTextures(1, &mat.id));
			}
		}
	}

	void Model3D::draw(Shader& shader, bool bBindMaterials /*= true*/) const
	{
		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			meshes[i].draw(shader, bBindMaterials);
		}
	}

	void Model3D::drawInstanced(Shader& shader, uint32_t instanceCount, bool bBindMaterials /*= true*/) const
	{
		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			meshes[i].drawInstanced(shader, instanceCount, bBindMaterials);
		}
	}


	/* must be called after ctor creates vao*/
	void Model3D::setInstancedModelMatricesData(glm::mat4* modelMatrices, uint32_t count)
	{
		for (Mesh3D& mesh : meshes)
		{
			mesh.setInstancedModelMatricesData(modelMatrices, count);
		}
	}

	AnimationData Model3D::startAnimation(uint32_t animationID, bool bLoop)
	{
		AnimationData animData;
		animData.animId = animationID;
		animData.bLoop = bLoop;
		animData.startTime = std::chrono::system_clock::now();
		return animData;
	}

	void Model3D::prepareAnimationsForData(AnimationData& inOut_AnimationData)
	{
		using std::chrono::duration_cast;
		using std::chrono::milliseconds;
		using std::chrono::nanoseconds;

		if (cachedScene && cachedScene->HasAnimations() && inOut_AnimationData.animId < cachedScene->mNumAnimations)
		{
			auto currentTimePoint = std::chrono::system_clock::now(); //#TODO: this may should be handled externally so we only make this system call once
			auto duration = duration_cast<milliseconds>(currentTimePoint - inOut_AnimationData.startTime); //use miliseconds to get milisecond precision
			float animCurSeconds = static_cast<float>(duration.count()) / 1000.0f;

			aiAnimation* anim = cachedScene->mAnimations[inOut_AnimationData.animId];
			assert(anim->mDuration != 0);
			assert(anim->mTicksPerSecond != 0);

			float animCurTicks = static_cast<float>(animCurSeconds * anim->mTicksPerSecond);

			//handle if current time is passed animation duration (doing this in seconds because of ease for timepoints
			//float animDuration = static_cast<float>(anim->mTicksPerSecond * anim->mDuration);
			float animDurationSecs = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);
			if (animCurSeconds > animDurationSecs)
			{
				if (inOut_AnimationData.bLoop)
				{
					animCurSeconds = std::fmod(animCurSeconds, animDurationSecs);
					//determine new start point for this animation duration 
					auto animMili = milliseconds(static_cast<long long>(1000 * animCurSeconds));
					auto newStart = currentTimePoint - animMili;
					inOut_AnimationData.startTime = newStart;
				}
				else
				{
					animCurSeconds = animDurationSecs;
				}
			}

			//now that looping time points are complete, convert new time to ticks 
			float animInTicks = static_cast<float>(animCurSeconds * anim->mTicksPerSecond);

			//create a mapping of channel name to animation node for quick lookup; (animation node contains bone transform data)
			std::map<std::string, aiNodeAnim*> nodeNameToChannel;
			for (uint32_t channel = 0; channel < anim->mNumChannels; ++channel)
			{
				nodeNameToChannel[std::string(anim->mChannels[channel]->mNodeName.C_Str())] = anim->mChannels[channel];
			}

			aiMatrix4x4 identity; //use aiMatrix internally to prevent tedious conversions
			calculateAnimationTransforms(anim, animInTicks, cachedScene->mRootNode, identity, nodeNameToChannel);
		}
	}

	void Model3D::sendBoneTransformsToShader(Shader& shader, const char* uniformMat4ArrayName)
	{
		//test attempt to fix mesh where there are no weighted bone
		//if (aiNode* node = findNamedNode("Armature", cachedScene->mRootNode)) //update shader if testing this
		//{
		//	shader.setUniformMatrix4fv("testTransform", 1, GL_FALSE, glm::value_ptr(toGlmMat4(node->mTransformation)));
		//}

		for (std::pair<const std::string, Bone>& KVPair : allBonesByName)
		{
			Bone& bone = KVPair.second;

			std::string uniformName = std::string(uniformMat4ArrayName) + "[" + std::to_string(bone.uniqueID) + "]";
			shader.setUniformMatrix4fv(uniformName.c_str(), 1, GL_FALSE, glm::value_ptr(toGlmMat4(bone.finalAnimatedBoneTransform)));
		}
	}

	void Model3D::calculateAnimationTransforms(
		aiAnimation* anim, float animationTimeInTicks,
		aiNode* node, const aiMatrix4x4& parentTransform, const std::map<std::string, aiNodeAnim*>& nodeNameToChannel)
	{
		aiMatrix4x4 nodeTransform;

		std::string nodeName = std::string(node->mName.C_Str());
		auto animNodePair = nodeNameToChannel.find(nodeName);
		if (animNodePair != nodeNameToChannel.end())
		{
			aiNodeAnim* animNode = animNodePair->second;

			aiMatrix4x4 translationMatrix = interpolatePositionKeys(animNode, animationTimeInTicks);
			aiMatrix4x4 rotationMatrix = interpolateRotationKeys(animNode, animationTimeInTicks);
			aiMatrix4x4 scaleMatrix = interpolateScaleKeys(animNode, animationTimeInTicks);

			nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
		}
		else
		{
			//use default transform since this node assists structuring
			nodeTransform = node->mTransformation;
		}

		aiMatrix4x4 nodeGlobalTransform = parentTransform * nodeTransform;
		//aiMatrix4x4 nodeGlobalTransform = nodeTransform;

		auto KV_NameBone = allBonesByName.find(nodeName);
		if (KV_NameBone != allBonesByName.end())
		{
			Bone& bone = KV_NameBone->second;
			const aiMatrix4x4& meshToBone = bone.meshToBoneTransform;
			//bone.finalAnimatedBoneTransform = nodeGlobalTransform * meshToBone;
			bone.finalAnimatedBoneTransform = inverseSceneTransform * nodeGlobalTransform * meshToBone;

			//bone.finalAnimatedBoneTransform = inverseSceneTransform * nodeGlobalTransform;
			//bone.finalAnimatedBoneTransform = nodeGlobalTransform * aiMatrix4x4(meshToBone).Inverse();
			//bone.finalAnimatedBoneTransform = inverseSceneTransform * nodeGlobalTransform;
		}

		for (uint32_t chld = 0; chld < node->mNumChildren; ++chld)
		{
			calculateAnimationTransforms(anim, animationTimeInTicks, node->mChildren[chld], nodeGlobalTransform, nodeNameToChannel);
		}
	}

	aiMatrix4x4 Model3D::interpolatePositionKeys(aiNodeAnim* animNode, float animationTimeInTicks)
	{
		if (animNode->mNumPositionKeys == 1)
		{ //edge case where there is only 1 animation key, just directly convert the translation of the bone by taking first key
			return toAiMat4(glm::translate(glm::mat4{ 1.0f }, toGlmVec3(animNode->mPositionKeys[0].mValue))); //slightly compressed, but just translates via GLM, then converts mat4 to aimat4. Real applications will probably just stick to 1 matrix library and not do these silly conversions
		}

		if (animNode->mNumPositionKeys > 0)
		{
			glm::uvec2 keyIdxs = searchForKeys(animNode->mPositionKeys, animNode->mNumPositionKeys, animationTimeInTicks);
			uint32_t firstKey = keyIdxs.r, secondKey = keyIdxs.g;

			double delta = animationTimeInTicks - animNode->mPositionKeys[firstKey].mTime;
			double interkeyTime = animNode->mPositionKeys[secondKey].mTime - animNode->mPositionKeys[firstKey].mTime;
			float interpAlpha = static_cast<float>(delta / interkeyTime);

			glm::vec3 first = toGlmVec3(animNode->mPositionKeys[firstKey].mValue);
			glm::vec3 second = toGlmVec3(animNode->mPositionKeys[secondKey].mValue);
			glm::vec3 toSecond = second - first;
			glm::vec3 interpolated = first + interpAlpha * toSecond;

			glm::mat4 transMat;
			transMat = glm::translate(transMat, interpolated);

			return toAiMat4(transMat);
		}

		return aiMatrix4x4{}; //documentation says no-arg constructor creates an idenity matrix
	}

	static aiMatrix4x4 rotation3x3To4x4Helper(const aiMatrix3x3& src)
	{
		aiMatrix4x4 result; //this is an identity matrix, the last column and last row are (0,0,0,1), which is what we need for those places in the rotation matrix.

		result.a1 = src.a1;
		result.a2 = src.a2;
		result.a3 = src.a3;

		result.b1 = src.b1;
		result.b2 = src.b2;
		result.b3 = src.b3;

		result.c1 = src.c1;
		result.c2 = src.c2;
		result.c3 = src.c3;

		return result;
	}

	aiMatrix4x4 Model3D::interpolateRotationKeys(aiNodeAnim* animNode, float animationTimeInTicks)
	{
		if (animNode->mNumRotationKeys == 1)
		{
			return rotation3x3To4x4Helper(animNode->mRotationKeys[0].mValue.GetMatrix());
		}
		if (animNode->mNumRotationKeys > 0)
		{

			glm::uvec2 keyIdxs = searchForKeys(animNode->mRotationKeys, animNode->mNumRotationKeys, animationTimeInTicks);
			uint32_t firstKey = keyIdxs.r, secondKey = keyIdxs.g;

			double delta = animationTimeInTicks - animNode->mRotationKeys[firstKey].mTime;
			double interkeyTime = animNode->mRotationKeys[secondKey].mTime - animNode->mRotationKeys[firstKey].mTime;
			float interpAlpha = static_cast<float>(delta / interkeyTime);

			aiQuaternion outInterpolatedQuat;
			const aiQuaternion firstKeyQuat = animNode->mRotationKeys[firstKey].mValue;
			const aiQuaternion secondKeyQuat = animNode->mRotationKeys[secondKey].mValue;
			aiQuaternion::Interpolate(outInterpolatedQuat, firstKeyQuat, secondKeyQuat, interpAlpha);
			//outInterpolatedQuat.Normalize();

			aiMatrix3x3 itpMat3 = outInterpolatedQuat.GetMatrix();
			return rotation3x3To4x4Helper(itpMat3);
		}
		return aiMatrix4x4{}; //return identity
	}

	aiMatrix4x4 Model3D::interpolateScaleKeys(aiNodeAnim* animNode, float animationTimeInTicks)
	{
		if (animNode->mNumScalingKeys == 1)
		{
			return toAiMat4(glm::scale(glm::mat4{ 1.0f }, toGlmVec3(animNode->mScalingKeys[0].mValue)));
		}

		if (animNode->mNumScalingKeys > 0)
		{
			glm::uvec2 keyIdxs = searchForKeys(animNode->mScalingKeys, animNode->mNumScalingKeys, animationTimeInTicks);
			uint32_t firstKey = keyIdxs.r, secondKey = keyIdxs.g;

			double delta = animationTimeInTicks - animNode->mScalingKeys[firstKey].mTime;
			double interkeyTime = animNode->mScalingKeys[secondKey].mTime - animNode->mScalingKeys[firstKey].mTime;
			float interpAlpha = static_cast<float>(delta / interkeyTime);

			glm::vec3 first = toGlmVec3(animNode->mScalingKeys[firstKey].mValue);
			glm::vec3 second = toGlmVec3(animNode->mScalingKeys[secondKey].mValue);
			glm::vec3 toSecond = second - first;
			glm::vec3 interpolated = first + interpAlpha * toSecond;

			glm::mat4 scaleMat;
			scaleMat = glm::scale(scaleMat, interpolated);

			return toAiMat4(scaleMat);
		}
		return aiMatrix4x4{}; //idenity matrix
	}

	glm::vec3 Model3D::toGlmVec3(const aiVector3D& aiVec3)
	{
		return glm::vec3(aiVec3.x, aiVec3.y, aiVec3.z);
	}

	aiVector3D Model3D::toAiVec3(const glm::vec3& glmVec3)
	{
		return aiVector3D(glmVec3.x, glmVec3.y, glmVec3.z);
	}

	aiMatrix4x4 Model3D::toAiMat4(glm::mat4 glmMat4)
	{
		aiMatrix4x4 result;

		//row 1
		result.a1 = glmMat4[0][0];
		result.a2 = glmMat4[1][0];
		result.a3 = glmMat4[2][0];
		result.a4 = glmMat4[3][0];

		//row 2
		result.b1 = glmMat4[0][1];
		result.b2 = glmMat4[1][1];
		result.b3 = glmMat4[2][1];
		result.b4 = glmMat4[3][1];

		//row 3
		result.c1 = glmMat4[0][2];
		result.c2 = glmMat4[1][2];
		result.c3 = glmMat4[2][2];
		result.c4 = glmMat4[3][2];

		//row 4
		result.d1 = glmMat4[0][3];
		result.d2 = glmMat4[1][3];
		result.d3 = glmMat4[2][3];
		result.d4 = glmMat4[3][3];

		return result;
	}

	glm::mat4 Model3D::toGlmMat4(aiMatrix4x4 aiMat4)
	{
		glm::mat4 result;
		//ai row 0
		result[0][0] = aiMat4.a1;
		result[1][0] = aiMat4.a2;
		result[2][0] = aiMat4.a3;
		result[3][0] = aiMat4.a4;

		//ai row 1
		result[0][1] = aiMat4.b1;
		result[1][1] = aiMat4.b2;
		result[2][1] = aiMat4.b3;
		result[3][1] = aiMat4.b4;

		//ai row 2
		result[0][2] = aiMat4.c1;
		result[1][2] = aiMat4.c2;
		result[2][2] = aiMat4.c3;
		result[3][2] = aiMat4.c4;

		//ai row 3
		result[0][3] = aiMat4.d1;
		result[1][3] = aiMat4.d2;
		result[2][3] = aiMat4.d3;
		result[3][3] = aiMat4.d4;

		return result;
	}

	std::tuple<glm::vec3, glm::vec3> Model3D::getAABB() const
	{
		return cachedAABB;
	}

	void Model3D::getMeshVAOS(std::vector<unsigned int>& outVAOs)
	{
		outVAOs.clear();
		for (Mesh3D& mesh : meshes)
		{
			GLuint meshVAO = mesh.getVAO();
			if (meshVAO) //if it is zero, we don't have an active opengl context or not yet initialized
			{
				outVAOs.push_back(meshVAO);
			}
		}
	}

	void Model3D::loadModel(std::string path)
	{
		//--------------------------------------------------------------------------------------------
		//const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		//const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals); //smooth normals required for bob model
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices); //smooth normals required for bob model, adding aiProcessJointIdenticavertices to better match tutorial
		//--------------------------------------------------------------------------------------------

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cerr << "Assimp: error importing model: ERROR::" << importer.GetErrorString() << std::endl;
			throw std::runtime_error("failed to load required model");
		}

		directory = path.substr(0, path.find_last_of('/'));
		this->cachedScene = scene;

		this->inverseSceneTransform = scene->mRootNode->mTransformation;
		this->inverseSceneTransform.Inverse();

		processNode(scene->mRootNode, scene);
	}

	void Model3D::processNode(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene, node));
		}

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	MaterialTexture generateDefaultNormalMapTextures()
	{
		AssetSystem& assetSystem = GameBase::get().getAssetSystem();

		MaterialTexture normalMap;
		normalMap.path = "GameData/engine_assets/NormalMap_Default.png";
		normalMap.type = "texture_normalmap";
		assetSystem.loadTexture(normalMap.path.c_str(), normalMap.id);

		return normalMap;
	}


	Mesh3D Model3D::processMesh(aiMesh* mesh, const aiScene* scene, const aiNode* parentNode)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<MaterialTexture> textures;
		std::vector<NormalData> normalData;
		std::vector<VertexBoneData> vertexBoneData;

		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;
			updateCachedAABB(vertex.position);

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

		for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
		{
			//loader forces faces to be triangles, so these indices should be valid for drawing triangles
			const aiFace& face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; ++j)
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
			if (normalMaps.size() == 0) 
			{
				normalMaps = { generateDefaultNormalMapTextures() };
			}

			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		}

		vertexBoneData.resize(vertices.size()); //default constructor all bone weights to 0

		std::map<std::string, Bone> nameToBoneMap;
		if (mesh->HasBones())
		{
			for (uint32_t boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
			{
				aiBone* bone = mesh->mBones[boneIdx];

				//cache bone information
				Bone cacheBone;
				cacheBone.name = bone->mName.C_Str();
				cacheBone.meshToBoneTransform = bone->mOffsetMatrix;
				cacheBone.weights.resize(0);
				cacheBone.weights.reserve(bone->mNumWeights);
				//cacheBone.uniqueID = static_cast<int>(allBones.size());
				cacheBone.uniqueID = static_cast<int>(allBonesByName.size());
				//meshes sharing bones need correct ids
				if (allBonesByName.find(cacheBone.name) != allBonesByName.end())
					cacheBone.uniqueID = allBonesByName[cacheBone.name].uniqueID;

				for (uint32_t wgt = 0; wgt < bone->mNumWeights; ++wgt)
				{
					cacheBone.weights.push_back(bone->mWeights[wgt]);

					uint32_t vertId = bone->mWeights[wgt].mVertexId;
					int curBone = vertexBoneData[vertId].currentBone;

					assert(curBone != MAX_NUM_BONES); //assert if we're about to go out of bounds

					//to vertex data, add an influencing bone and its weight; these will be used to lookup into bone transforms array
					vertexBoneData[vertId].boneWeights[curBone] = bone->mWeights[wgt].mWeight;
					vertexBoneData[vertId].boneIds[curBone] = cacheBone.uniqueID;
					vertexBoneData[vertId].currentBone++;
				}

				//#todo: may need to switch these over the shared pointers since each data structure has its own copy of its bone
				if (nameToBoneMap.find(cacheBone.name) == nameToBoneMap.end())
					nameToBoneMap[cacheBone.name] = cacheBone;
				else
					std::cerr << "duplicate bone " << cacheBone.name << " found when loading mesh" << std::endl;

				//globally cache bone
				if (allBonesByName.find(cacheBone.name) == allBonesByName.end())
					allBonesByName[cacheBone.name] = cacheBone; //there probably needs to be a different struct for the global bones, since data like "cachedWeights' is not relevant not accurate for all models; not sure but it seems meshToBone will be dependent upon mesh and cannot be used globally
				else
					std::cerr << "duplicate global bone " << cacheBone.name << " found when loading mesh" << std::endl;
				//allBones.push_back(cacheBone);
				markNodesForBone(cacheBone.name);
			}
		}

		return Mesh3D(vertices, textures, indices, normalData, vertexBoneData, nameToBoneMap);
	}

	std::vector<MaterialTexture> Model3D::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
	{
		std::vector<MaterialTexture> textures;

		uint32_t textureCount = mat->GetTextureCount(type);
		for (uint32_t i = 0; i < textureCount; i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			std::string relativePath = std::string(str.C_Str());

			std::string filepath = directory + std::string("/") + relativePath;

			bool bShouldSkip = false;
			for (uint32_t textureIdx = 0; textureIdx < texturesLoaded.size(); ++textureIdx)
			{
				if (texturesLoaded[textureIdx].path == relativePath)
				{
					//already loaded this texture, just the cached texture information
					textures.push_back(texturesLoaded[textureIdx]);
					bShouldSkip = true;
					break;
				}
			}

			if (!bShouldSkip)
			{
				MaterialTexture texture;
				texture.id = Utils::loadTextureToOpengl(filepath.c_str());
				texture.type = typeName;
				texture.path = relativePath;
				textures.push_back(texture);

				//cache for later texture loads
				texturesLoaded.push_back(texture);
			}
		}
		return textures;
	}

	void Model3D::updateCachedAABB(const glm::vec3& vertexPosition)
	{
		glm::vec3& min = std::get<0>(cachedAABB);
		glm::vec3& max = std::get<1>(cachedAABB);

		if (min.x > vertexPosition.x) min.x = vertexPosition.x;
		if (min.y > vertexPosition.y) min.y = vertexPosition.y;
		if (min.z > vertexPosition.z) min.z = vertexPosition.z;

		if (max.x < vertexPosition.x) max.x = vertexPosition.x;
		if (max.y < vertexPosition.y) max.y = vertexPosition.y;
		if (max.z < vertexPosition.z) max.z = vertexPosition.z;
	}

	void Model3D::markNodesForBone(const std::string& boneName)
	{
		//this can probably be optimized, instead do a single walk over the entire graph of the model when each node is arived, check data structure for referencing bone
		aiNode* node = findNamedNode(boneName.c_str(), cachedScene->mRootNode);

		//assimp documentation recommends stopping once we reach the node for the owning mesh, but i'm not sure that things are properly managed to start animation updates from mesh nodes so I'm deferring siad check for now; if I add this later, said node should be available at this function's callsite
		while (node && node != cachedScene->mRootNode) //root node should never be null
		{
			skeletonRelevantNode.insert(node);
			node = node->mParent;
		}
	}

	aiNode* Model3D::findNamedNode(const char* name, aiNode* startNode)
	{
		/** Essentially BFS to find a named node; bones/animations should have unique names according to assimp documentation */
		if (strcmp(startNode->mName.C_Str(), name) == 0)
		{
			return startNode;
		}

		for (uint32_t child = 0; child < startNode->mNumChildren; ++child)
		{
			if (aiNode* namedNode = findNamedNode(name, startNode->mChildren[child]))
			{
				return namedNode;
			}
		}
		return nullptr;
	}



}