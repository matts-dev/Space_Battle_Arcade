#pragma once


#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "../RemoveSpecialMemberFunctionUtils.h"
#include "../../GameFramework/SAGameEntity.h"

namespace SA
{
	//forward declarations
	class Window;
	class Model3D;
	class Shader;


	///////////////////////////////////////////////////////////////////////
	// Simple Shape Base Class
	///////////////////////////////////////////////////////////////////////
	class ShapeMesh : public GameEntity, public RemoveCopies, public RemoveMoves
	{
	public:
		ShapeMesh() = default;
		virtual ~ShapeMesh() = default;
		virtual void render() const = 0;
		virtual void instanceRender(int instanceCount) const = 0;

		void handleWindowLosingOpenglContext(const sp<Window>& window);
		void handleWindowAcquiredOpenglContext(const sp<Window>& window);
		virtual const std::vector<unsigned int>& getVAOs() = 0;
	protected:
		bool hasAcquiredOpenglResources() { return bAcquiredOpenglResources; }
	private:
		bool bAcquiredOpenglResources = false;
		void releaseOpenGLResources();
		void acquireOpenGLResources();

	protected:
		virtual void postConstruct() override;

	private: //subclass provided
		virtual void onReleaseOpenGLResources() = 0;
		virtual void onAcquireOpenGLResources() = 0;
	};

	///////////////////////////////////////////////////////////////////////
	// Textured Sphere
	///////////////////////////////////////////////////////////////////////
	class SphereMeshTextured final : public ShapeMesh
	{

	public:
		SphereMeshTextured(float tolerance = 0.014125375f);
		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;

		virtual const std::vector<unsigned int>& getVAOs() override { return vaos; }

	private:
		virtual void onAcquireOpenGLResources() override;
		virtual void onReleaseOpenGLResources() override;
		void buildMesh(float TOLERANCE);

		void configureDataForOpenGL();

		//helpers
		static int calculateNumFacets(float tolerance);
		static void generateUnitSphere(float tolerance, std::vector<glm::vec3>& vertList, std::vector<glm::vec3>& normalList, std::vector<glm::vec2>& textureCoordsList, std::vector<int>& rowIterations_numFacets);
		static glm::vec3 copyAndRotatePointZAxis(const glm::vec3 toCopy, float rotationDegrees);
		static glm::vec3& rotatePointZAxis(glm::vec3& point, float rotationDegrees);
		static glm::vec3 copyAndRotatePointXAxis(const glm::vec3& toCopy, float rotationDegrees);

	private:
		float tolerance;

		std::vector<float> vertPositions;
		std::vector<float> normals;
		std::vector<float> textureCoords;
		std::vector<unsigned int> triangleElementIndices;

		std::vector<unsigned int> vaos;
		GLuint vao = 0;
		GLuint vboPositions = 0;
		GLuint vboNormals = 0;
		GLuint vboTexCoords = 0;
		GLuint ebo = 0;

	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Model 3D Wrapper
	//
	// Warning, if you're wrapping a model to be used with the particle system: the particle
	// system will iterate over the VAOs and inject vertex attributes for instanced rendering
	// so, be sure not to use any of the reserved vertex array attributes for instanced
	// particle rendering.
	/////////////////////////////////////////////////////////////////////////////////////

	class Model3DWrapper final : public ShapeMesh
	{
	public:
		Model3DWrapper(const sp<Model3D>& inModel, const sp<Shader>& inShader);

		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;

		virtual const std::vector<unsigned int>& getVAOs() override;
		virtual void onReleaseOpenGLResources() override;
		virtual void onAcquireOpenGLResources() override;

		//assert Ownership to prevent RAII clean up
		sp<Model3D> modelOwnership;
		sp<Shader> shaderOwnership;

		std::vector<unsigned int> vaos;

		//no copies/moves allowed for ShapeBase; so safe to hold these references
		Model3D& model;
		Shader& shader;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// Textured Quad
	/////////////////////////////////////////////////////////////////////////////////////
	class TexturedQuad final : public ShapeMesh
	{
	public:
		TexturedQuad();

		virtual void render() const override;
		virtual void instanceRender(int instanceCount) const override;
		virtual const std::vector<unsigned int>& getVAOs() override;
	private:
		virtual void onAcquireOpenGLResources() override;
		virtual void onReleaseOpenGLResources() override;
	private:
		std::vector<float> vertices;
		std::vector<unsigned int> indices;
		std::vector<unsigned int> vaos;

		unsigned int vao;
		unsigned int ebo;
		unsigned int vbo;
	};
}


