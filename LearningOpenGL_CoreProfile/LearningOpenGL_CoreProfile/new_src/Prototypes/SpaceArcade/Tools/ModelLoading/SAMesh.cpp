#include "SAMesh.h"
#include "SAModel.h"
#include <iostream>
#include <assimp/scene.h>
#include <gtc/type_ptr.hpp>
#include "../../Rendering/SAShader.h"
#include "../../Rendering/OpenGLHelpers.h"


namespace SA
{

	void Mesh3D::setupMesh()
	{
		ec(glBindVertexArray(0));
		ec(glGenVertexArrays(1, &VAO));
		ec(glGenBuffers(1, &VBO));
		ec(glGenBuffers(1, &VBO_TANGENTS));
		ec(glGenBuffers(1, &VBO_BONE_IDS));
		ec(glGenBuffers(1, &VBO_BONE_WEIGHTS));
		ec(glGenBuffers(1, &EAO));

		ec(glBindVertexArray(VAO));

		ec(glBindBuffer(GL_ARRAY_BUFFER, VBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW));

		ec(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO));
		ec(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW));

		//enable vertex data 
		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));

		//enable normal data
		ec(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal))));
		ec(glEnableVertexAttribArray(1));

		//enable texture coordinate data
		ec(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, textureCoords))));
		ec(glEnableVertexAttribArray(2));


		//crashes on glBufferData when all data contained in a single VBO; spliting up by using another VBO
		//enable tangent storage
		ec(glBindBuffer(GL_ARRAY_BUFFER, VBO_TANGENTS));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(NormalData) * normalData.size(), &normalData[0], GL_STATIC_DRAW));
		ec(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(NormalData), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(3));

		//enable bitangent
		ec(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(NormalData), reinterpret_cast<void*>(offsetof(NormalData, bitangent))));
		ec(glEnableVertexAttribArray(4));

		//for compactness, we're going to pack all 4 bones into a single vec4
		//if we chose to do more than 4 bone, this will cause issues with bone code below since glVertAttribPointer variants can only load into vec1-vec4
		assert(MAX_NUM_BONES <= 4 && MAX_NUM_BONES > 0);

		ec(glBindBuffer(GL_ARRAY_BUFFER, VBO_BONE_IDS));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(int) * boneIdData.size(), &boneIdData[0], GL_STATIC_DRAW));
		ec(glVertexAttribIPointer(5, MAX_NUM_BONES, GL_INT, sizeof(int) * MAX_NUM_BONES, reinterpret_cast<void*>(0))); ///notice that this is glVertiedAttribIPointer for integers
		ec(glEnableVertexAttribArray(5));

		ec(glBindBuffer(GL_ARRAY_BUFFER, VBO_BONE_WEIGHTS));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * boneWeightData.size(), &boneWeightData[0], GL_STATIC_DRAW));
		ec(glVertexAttribPointer(6, MAX_NUM_BONES, GL_FLOAT, GL_FALSE, sizeof(float) * MAX_NUM_BONES, reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(6));
	}

	Mesh3D::Mesh3D(
		std::vector<Vertex>& vertices,
		std::vector<MaterialTexture>& textures,
		std::vector<unsigned int>& indices,
		std::vector<NormalData>& normalData,
		std::vector<VertexBoneData>& vertexBoneData,
		std::map<std::string, Bone>& nameToBoneMap
	) :
		vertices(vertices),
		textures(textures),
		indices(indices),
		normalData(normalData),
		nameToBoneMap(nameToBoneMap)
	{
		//each vertex has a fixed number of bone-lookup data used to read from the bone transforms and weight their influence 
		for (size_t vert = 0; vert < vertexBoneData.size(); vert++)
		{
			for (size_t bone = 0; bone < MAX_NUM_BONES; ++bone)
			{
				//the number of bones influencing a vertex will need to be respected when loading this into vbo
				boneIdData.push_back(vertexBoneData[vert].boneIds[bone]);
				boneWeightData.push_back(vertexBoneData[vert].boneWeights[bone]);
			}
		}

		setupMesh();
	}

	Mesh3D::~Mesh3D()
	{
		//be careful, copy ctor kills buffers silently if these are uncommented (currently happening in instancing tutorial)
		//ec(glDeleteBuffers(1, &VBO));
		//ec(glDeleteBuffers(1, &EAO));
		//ec(glDeleteBuffers(1, &modelVBO));
		//ec(glDeleteVertexArrays(1, &VAO));
	}

	void Mesh3D::draw(Shader& shader, bool bBindMaterials /*= true*/) const
	{
		using std::string;
		if (bBindMaterials)
		{
			unsigned int diffuseTextureNumber = 0;
			unsigned int specularTextureNumber = 0;
			unsigned int normalMapTextureNumber = 0;
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
				else if (uniformName == "texture_normalmap")
				{
					uniformName = string("material.") + uniformName + std::to_string(normalMapTextureNumber);
					++normalMapTextureNumber;
				}
				ec(glActiveTexture(currentTextureUnit));
				ec(glBindTexture(GL_TEXTURE_2D, textures[i].id));
				shader.setUniform1i(uniformName.c_str(), currentTextureUnit - GL_TEXTURE0);
				++currentTextureUnit;
			}
		}

		//draw mesh
		ec(glBindVertexArray(VAO));
		ec(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0));
		ec(glBindVertexArray(0));
	}

	void Mesh3D::drawInstanced(Shader& shader, unsigned int instanceCount, bool bBindMaterials/*=true*/) const
	{
		using std::string;

		unsigned int diffuseTextureNumber = 0;
		unsigned int specularTextureNumber = 0;
		unsigned int normalMapTextureNumber = 0;
		unsigned int currentTextureUnit = GL_TEXTURE0;

		if (bBindMaterials)
		{
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
				else if (uniformName == "texture_normalmap")
				{
					uniformName = string("material.") + uniformName + std::to_string(normalMapTextureNumber);
					++normalMapTextureNumber;
				}
				ec(glActiveTexture(currentTextureUnit));
				ec(glBindTexture(GL_TEXTURE_2D, textures[i].id));
				shader.setUniform1i(uniformName.c_str(), currentTextureUnit - GL_TEXTURE0);
				++currentTextureUnit;
			}
		}

		//draw mesh
		ec(glBindVertexArray(VAO));
		ec(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO));
		ec(glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceCount));
		ec(glBindVertexArray(0));
	}

	/** This really should be a private function, making visible for instancing tutorial */
	GLuint Mesh3D::getVAO()
	{
		return VAO;
	}

	void Mesh3D::setInstancedModelMatrixVBO(GLuint modelVBO)
	{
		this->modelVBO = modelVBO;
	}

	void Mesh3D::setInstancedModelMatricesData(glm::mat4* modelMatrices, unsigned int count)
	{
		ec(glBindVertexArray(VAO));

		if (modelVBO == 0)
		{
			ec(glGenBuffers(1, &modelVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, modelVBO));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * count, &modelMatrices[0], GL_STATIC_DRAW));
			ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(0)));
			ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(sizeof(glm::vec4))));
			ec(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(2 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void*>(3 * sizeof(glm::vec4))));
			ec(glEnableVertexAttribArray(3));
			ec(glEnableVertexAttribArray(4));
			ec(glEnableVertexAttribArray(5));
			ec(glEnableVertexAttribArray(6));
			ec(glVertexAttribDivisor(3, 1));
			ec(glVertexAttribDivisor(4, 1));
			ec(glVertexAttribDivisor(5, 1));
			ec(glVertexAttribDivisor(6, 1));
		}
		else
		{
			std::cerr << "instanced model matrices VBO already exists" << std::endl;
		}

		ec(glBindVertexArray(0));
	}

}