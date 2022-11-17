#pragma once
#include <vector>
#include <assimp\scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <stdexcept>

#include <set>
#include <chrono>
#include "..\..\Rendering\SAShader.h"
#include "SAMesh.h"

namespace SA
{

	/** This probably isn't the ideal system for animations, but more a first pass to get interpolation working */
	struct AnimationData
	{
		uint32_t animId = 0;
		bool bLoop = true;
		std::chrono::time_point<std::chrono::system_clock> startTime; //this value will change if looping is enabled
	};

	class Model3D
	{
		friend class AssetSystem;
	public:

		Model3D(const char* path);
		~Model3D();
		void draw(Shader& shader, bool bBindMaterials = true) const;
		void drawInstanced(Shader& shader, uint32_t instanceCount, bool bBindMaterials = true) const;

		void setInstancedModelMatricesData(glm::mat4* modelMatrices, uint32_t count);

		/** animation interface */
		void prepareAnimationsForData(AnimationData& animationData);
		void sendBoneTransformsToShader(Shader& shader, const char* uniformMat4ArrayName);
		AnimationData startAnimation(uint32_t animationID, bool bLoop);

		//helper conversions
		static glm::vec3 toGlmVec3(const aiVector3D& aiVec3);
		static aiVector3D toAiVec3(const glm::vec3& glmVec3);
		static aiMatrix4x4 toAiMat4(glm::mat4 glmMat4);
		static glm::mat4 toGlmMat4(aiMatrix4x4 aiMat4);

		/** min=std::get<0>(aabb); max=std::get<1>(aabb);*/
		std::tuple<glm::vec3, glm::vec3> getAABB() const;

		void getMeshVAOS(std::vector<unsigned int>& outVAOs);
		const std::vector<Mesh3D>& getMeshes() const { return meshes; };

	private://model/mesh methods

		void loadModel(std::string path);

		void processNode(aiNode* node, const aiScene* scene);
		Mesh3D processMesh(aiMesh* mesh, const aiScene* scene, const aiNode* parentNode);

		std::vector<MaterialTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

	private: 
		void releaseGPUData();
		void updateCachedAABB(const glm::vec3& vertexPosition);

	private: //bone methods
		void markNodesForBone(const std::string& boneName);
		aiNode* findNamedNode(const char* name, aiNode* startNode);
		void calculateAnimationTransforms(aiAnimation* anim, float animationTimeInTicks, aiNode* node, const aiMatrix4x4& parentTransform, const std::map<std::string, aiNodeAnim*>& nodeNameToChannel);

		template<typename KEY_TYPE> //there are vector keys and quaternion keys, code is the same for both
		glm::uvec2 searchForKeys(KEY_TYPE* keys, uint32_t numKeys, float animationTimeInSec);

		aiMatrix4x4 interpolatePositionKeys(aiNodeAnim* animNode, float animationTimeInTicks);
		aiMatrix4x4 interpolateRotationKeys(aiNodeAnim* animNode, float animationTimeInTicks);
		aiMatrix4x4 interpolateScaleKeys(aiNodeAnim* animNode, float animationTimeInTicks);

	private: //members
		std::string directory;
		bool bGPUReleased = false;
		std::vector<Mesh3D> meshes;
		std::vector<MaterialTexture> texturesLoaded;
		std::map<std::string, Bone> allBonesByName;


		/** This dtor will clean up graphs loaded; thus, this is a member variable to make lifetime of assimp graphs the same as the instance of this object.*/
		Assimp::Importer importer;
		std::set<aiNode*> skeletonRelevantNode;		//memory managed by importer
		const aiScene* cachedScene;					//memory managed by importer
		aiMatrix4x4 inverseSceneTransform;

		std::tuple<glm::vec3, glm::vec3> cachedAABB;
	};

















	// --------------- template definitions --------------------
	template<typename KEY_TYPE> //there are vector keys and quaternion keys, code is the same for both
	glm::uvec2 Model3D::searchForKeys(KEY_TYPE* keys, uint32_t numKeys, float animationTimeInTicks)
	{
		if (numKeys == 1)
		{
			return glm::uvec2(0, 0);
		}

		//in real applications we probably want a way to speed up this us so that we don't have to search all frames.
		//perhaps binary search on time, or subdivide all animations into ticks based on animations total time with correct frames for given tick
		for (uint32_t animKey = 0; animKey < numKeys; ++animKey)
		{
			bool bCurrKeyBeforeTime = keys[animKey].mTime <= animationTimeInTicks; //<= to catch edge case of animation time: 0.0f
			bool bNextKeyAfterTime = keys[animKey + 1].mTime >= animationTimeInTicks; //>= to cover edge case where time is exactly the last frame
			if (bCurrKeyBeforeTime && bNextKeyAfterTime)
			{
				return glm::uvec2(animKey, animKey + 1);
			}
		}

		assert(0); //we should never "not find" a pair of keys 	
		throw std::runtime_error("Model::searchForKeys did not find keys in animation");
	}

}