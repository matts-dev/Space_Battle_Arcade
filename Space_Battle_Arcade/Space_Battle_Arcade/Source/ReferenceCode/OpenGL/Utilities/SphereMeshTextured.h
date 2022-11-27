#pragma once
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include "Mesh.h"

class SphereMeshTextured final : public Mesh
{
private:
	std::vector<float> vertPositions;
	std::vector<float> normals;
	std::vector<float> textureCoords;
	std::vector<unsigned int> triangleElementIndices;

	GLuint vao = 0;
	GLuint vboPositions = 0;
	GLuint vboNormals = 0;
	GLuint vboTexCoords = 0;
	GLuint ebo = 0;

public:
	SphereMeshTextured(float tolerance = 0.014125375f);
	~SphereMeshTextured();
	void render();

	//special member functions
	SphereMeshTextured(const SphereMeshTextured& copy) = delete;
	SphereMeshTextured(SphereMeshTextured&& move) = delete;
	SphereMeshTextured operator=(const SphereMeshTextured& copy) = delete;
	SphereMeshTextured operator=(SphereMeshTextured&& move) = delete;

private:
	void buildMesh(float TOLERANCE);

	void configureDataForOpenGL();

	//helpers
	static int calculateNumFacets(float tolerance);
	static void generateUnitSphere(float tolerance, std::vector<glm::vec3>& vertList, std::vector<glm::vec3>& normalList, std::vector<glm::vec2>& textureCoordsList, std::vector<int>& rowIterations_numFacets);
	static glm::vec3 copyAndRotatePointZAxis(const glm::vec3 toCopy, float rotationDegrees);
	static glm::vec3& rotatePointZAxis(glm::vec3& point, float rotationDegrees);
	static glm::vec3 copyAndRotatePointXAxis(const glm::vec3& toCopy, float rotationDegrees);

};

