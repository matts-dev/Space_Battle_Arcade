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


	///////////////////////////////////////////////////////////////////////
	// Simple Shade Base Class
	///////////////////////////////////////////////////////////////////////
	class ShapeMesh : public GameEntity, public RemoveCopies, public RemoveMoves
	{
	public:
		ShapeMesh() = default;
		virtual ~ShapeMesh() = default;
		virtual void render() = 0;
		virtual void instanceRender(int instanceCount) = 0;

		void handleWindowLosingOpenglContext(const sp<Window>& window);
		void handleWindowAcquiredOpenglContext(const sp<Window>& window);
		virtual unsigned int getVAO() = 0;
	private:
		bool bAcquiredOpenglResources = false;
		void releaseOpenGLResources();
		void acquireOpenGLResources();

	private:
		virtual void postConstruct() override;

	private: //subclass provided
		virtual void onReleaseOpenGLResources() = 0;
		virtual void onAcquireOpenGLResources() = 0;

	private:

	};

	///////////////////////////////////////////////////////////////////////
	// Textured Sphere
	///////////////////////////////////////////////////////////////////////
	class SphereMeshTextured final : public ShapeMesh
	{

	public:
		SphereMeshTextured(float tolerance = 0.014125375f);
		virtual void render() override;
		virtual void instanceRender(int instanceCount) override;

		virtual unsigned int getVAO() override { return vao; }

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

		GLuint vao = 0;
		GLuint vboPositions = 0;
		GLuint vboNormals = 0;
		GLuint vboTexCoords = 0;
		GLuint ebo = 0;

	};


}


