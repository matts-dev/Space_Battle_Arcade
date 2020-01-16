#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include<cstddef> //offsetof macro
#include <assimp/mesh.h>
#include <map>
#include <assimp/scene.h>
#include "../../Rendering/SAShader.h"

namespace SA
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 textureCoords;
		//putting tangent/bitangent here causes crashes on glBufferData, probably because data too large
	};

	struct NormalData
	{
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	constexpr unsigned int MAX_NUM_BONES = 4; //changing this value will require changes in the way we store vertex data, it should only be [1,4] range
	struct VertexBoneData
	{
		//0 initialize weights
		unsigned int boneIds[MAX_NUM_BONES] = { 0 };
		float boneWeights[MAX_NUM_BONES] = { 0 };
		int currentBone = 0; //represents bones [0,3], this is incremented as we process; perhaps a better name is nextBone?
	};

	struct MaterialTexture
	{
		unsigned int id;
		std::string type;
		std::string path;
	};

	struct Bone
	{
		std::string name;
		std::vector<aiVertexWeight> weights;
		aiMatrix4x4 meshToBoneTransform;
		aiMatrix4x4 finalAnimatedBoneTransform;
		int uniqueID = -1;
	};

	class Mesh3D
	{
	public:
		Mesh3D(
			std::vector<Vertex>& vertices,
			std::vector<MaterialTexture>& textures,
			std::vector<unsigned int>& indices,
			std::vector<NormalData>& normalData,
			std::vector<VertexBoneData>& vertexBoneData,
			std::map<std::string, Bone>& nameToBoneMap
		);
		~Mesh3D();

		void draw(Shader& shader, bool bBindMaterials = true) const;
		void drawInstanced(Shader& shader, unsigned int instanceCount, bool bBindTextures=true) const;
		GLuint getVAO();
		void setInstancedModelMatrixVBO(GLuint modelVBO);
		void setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count);

		const std::vector<Vertex>& getVertices() const { return vertices; }
		const std::vector<unsigned int>& getIndices() const { return indices; }
	private:
		//vertex data
		std::vector<Vertex> vertices;
		std::vector<MaterialTexture> textures;
		std::vector<unsigned int> indices;
		std::vector<NormalData> normalData;
		std::vector<int> boneIdData;
		std::vector<float> boneWeightData;

		GLuint VAO, VBO, VBO_TANGENTS, VBO_BONE_IDS, VBO_BONE_WEIGHTS, EAO;
		GLuint modelVBO = 0;
		std::tuple<glm::vec3, glm::vec3> aabb;

		void setupMesh();

		//helpers
		std::map<std::string, Bone>& nameToBoneMap;
	};
}