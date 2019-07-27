#include "SimpleShapes.h"
#include "..\..\GameFramework\SAGameBase.h"
#include "..\..\GameFramework\SAWindowSystem.h"
#include "..\..\Rendering\SAWindow.h"
#include "..\..\Rendering\OpenGLHelpers.h"

namespace SA
{

	///////////////////////////////////////////////////////////////////////
	// Shape Mesh
	///////////////////////////////////////////////////////////////////////

	void ShapeMesh::handleWindowLosingOpenglContext(const sp<Window>& window)
	{
		releaseOpenGLResources();
	}

	void ShapeMesh::handleWindowAcquiredOpenglContext(const sp<Window>& window)
	{
		acquireOpenGLResources();
	}

	void ShapeMesh::releaseOpenGLResources()
	{
		if (bAcquiredOpenglResources)
		{
			onReleaseOpenGLResources();
			bAcquiredOpenglResources = false;
		}
	}

	void ShapeMesh::acquireOpenGLResources()
	{
		if (!bAcquiredOpenglResources)
		{
			onAcquireOpenGLResources();
			bAcquiredOpenglResources = true;
		}
	}

	void ShapeMesh::postConstruct()
	{
		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		//windowSystem.onWindowLosingOpenglContext
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &ShapeMesh::handleWindowLosingOpenglContext);
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &ShapeMesh::handleWindowAcquiredOpenglContext);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleWindowAcquiredOpenglContext(primaryWindow);
		}
	}

	///////////////////////////////////////////////////////////////////////
	// Textured Sphere
	///////////////////////////////////////////////////////////////////////


	SphereMeshTextured::SphereMeshTextured(float tolerance)
	{
		this->tolerance = tolerance;
	}

	void SphereMeshTextured::buildMesh(float TOLERANCE)
	{
		using namespace glm;

		std::vector<vec3> sphereVerts;
		std::vector<vec3> sphereNormals;
		std::vector<vec2> sphereTextureCoords;

		std::vector<int> rowIterations_numFacets;
		generateUnitSphere(TOLERANCE, sphereVerts, sphereNormals, sphereTextureCoords, rowIterations_numFacets);

		int latitudeNum = rowIterations_numFacets[0];
		int numFacetsInCircle = rowIterations_numFacets[1];
		//numFacetsInCircle = Math.max(numFacetsInCircle, 4); 

		// for every facet in circle, we have a strip of the same number of facets.
		// Since we have triangles for every two facet layers, there are numFacetsInCircle - 1 triangle strips.
		// Every facet contributes to 2 triangles.
		int numTriangles = numFacetsInCircle * (latitudeNum - 1) * 2;

		std::vector<float> verts(sphereVerts.size() * 3);
		std::vector<float> normals(sphereNormals.size() * 3);
		std::vector<float> uvs(sphereTextureCoords.size() * 2);
		std::vector<unsigned int> triangles(numTriangles * 3);

		//whole vertex offset to avoid multiplications within the indices of the array. 
		int vertOffset = 0;
		int uvOffset = 0;
		int triangleElementOffset = 0;

		//for (int verticalIdx = 0; verticalIdx < latitudeNum; ++verticalIdx)
		for (int verticalIdx = 0; verticalIdx < latitudeNum; ++verticalIdx)
		{
			int rowOffset = verticalIdx * numFacetsInCircle;
			//for (int horriIdx = 0; horriIdx < numFacetsInCircle; ++horriIdx)
			for (int horriIdx = 0; horriIdx < numFacetsInCircle; ++horriIdx)
			{
				int eleIdx = rowOffset + horriIdx;
				vec3 vertex = sphereVerts[eleIdx];
				vec3 normal = sphereNormals[eleIdx];
				vec2 uv = sphereTextureCoords[eleIdx];

				//vertex and normals share same offset, so it is updated after.
				verts[vertOffset] = vertex.x;
				verts[vertOffset + 1] = vertex.y;
				verts[vertOffset + 2] = vertex.z;
				normals[vertOffset] = normal.x;
				normals[vertOffset + 1] = normal.y;
				normals[vertOffset + 2] = normal.z;
				uvs[uvOffset] = uv.x;
				uvs[uvOffset + 1] = uv.y;
				vertOffset += 3;
				uvOffset += 2;

				//connect layer's triangles
				if (verticalIdx != 0)
				{ //first layer has nothing to connect to.
				  // How faces between rings are set up as triangles.
				  // r1v1 --- r1v2   triangle1 = r2v1, r1v2, r2v2
				  //   |   /    |    triangle2 = r2v1, r1v1, r1v2
				  // r2v1 --- r2v2   point rotation matters for OpenGL
					int r2v1 = horriIdx + rowOffset;
					int r2v2 = (horriIdx + 1) % numFacetsInCircle + rowOffset;
					int r1v1 = horriIdx + (rowOffset - numFacetsInCircle);
					int r1v2 = (horriIdx + 1) % numFacetsInCircle + (rowOffset - numFacetsInCircle);

					// triangle 1
					triangles[triangleElementOffset] = r2v1;
					triangles[triangleElementOffset + 1] = r2v2;
					triangles[triangleElementOffset + 2] = r1v2;

					// triangle 2
					triangles[triangleElementOffset + 3] = r1v1;
					triangles[triangleElementOffset + 4] = r2v1;
					triangles[triangleElementOffset + 5] = r1v2;
					triangleElementOffset += 6;
				}
			}
		}

		this->vertPositions = verts;
		this->normals = normals;
		this->textureCoords = uvs;
		this->triangleElementIndices = triangles;

		configureDataForOpenGL();
	}

	void SphereMeshTextured::configureDataForOpenGL()
	{
		//You can use multiple VBOs within a single VAO. For simplicity that is what I am doing.
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vboPositions);
		glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertPositions.size(), vertPositions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &vboNormals);
		glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(1);

		glGenBuffers(1, &vboTexCoords);
		glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * textureCoords.size(), textureCoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(2);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * triangleElementIndices.size(), triangleElementIndices.data(), GL_STATIC_DRAW);

		//prevent other calls from corrupting this VAO state
		glBindVertexArray(0);
	}

	int SphereMeshTextured::calculateNumFacets(float tolerance)
	{
		return static_cast<int>(round((glm::pi<float>() / (4 * tolerance))));
	}

	void SphereMeshTextured::generateUnitSphere(float tolerance,
		std::vector<glm::vec3>& vertList,
		std::vector<glm::vec3>& normalList,
		std::vector<glm::vec2>& textureCoordsList,
		std::vector<int>& rowIterations_numFacets)
	{
		using namespace glm;

		tolerance = clamp(tolerance, 0.01f, 0.2f);
		int numFacets = calculateNumFacets(tolerance);
		//subtract 1 so that textures will look correct, we basically will overlap the last vertices with the first so coordinates don't abruptly got from ~0.97 to 0
		float rotationDegrees = 360.0f / (numFacets - 1);
		vec3 srcPoint(1, 0, 0);

		//generate a 2D grid of vertices that will be mapped to a sphere.
		int numRows = (numFacets / 2) + 1;
		float rowRotationDegrees = 180.0f / (numRows - 1);

		glm::vec2 uvStart(0, 1);

		float uDelta = 1.0f / (numFacets - 1);
		float vDelta = -1.0f / (numRows - 1);

		for (int row = 0; row < numRows; ++row)
		{
			vec3 latitudePnt = copyAndRotatePointZAxis(srcPoint, row * rowRotationDegrees);
			float vCoord = uvStart.y + row * vDelta;

			for (int col = 0; col < numFacets; ++col)
			{
				//for a unit sphere the normals match the vertex locations.
				vec3 finalPnt = copyAndRotatePointXAxis(latitudePnt, col * rotationDegrees);
				float uCoord = uvStart.x + col * uDelta;

				vertList.push_back(finalPnt);
				normalList.push_back(finalPnt);
				textureCoordsList.push_back(vec2(uCoord, vCoord));
			}
		}

		//re-orient sphere so poles are at top and bottom
		for (unsigned int i = 0; i < vertList.size(); ++i)
		{
			vec3& vertex = vertList[i];
			vec3& normal = normalList[i];

			rotatePointZAxis(vertex, 90);
			normal = vertex;
		}

		rowIterations_numFacets.clear();
		rowIterations_numFacets.push_back(numRows);
		rowIterations_numFacets.push_back(numFacets);
	}

	glm::vec3 SphereMeshTextured::copyAndRotatePointZAxis(const glm::vec3 toCopy, float rotationDegrees)
	{
		using namespace glm;

		vec3 copy(toCopy);
		//expecting RVO to occur
		return rotatePointZAxis(copy, rotationDegrees);
	}

	glm::vec3& SphereMeshTextured::rotatePointZAxis(glm::vec3& point, float rotationDegrees)
	{
		using namespace glm;

		glm::mat4 rotationMatrix(1.f);
		rotationMatrix = rotate(rotationMatrix, glm::radians(rotationDegrees), vec3(0, 0, 1.f));

		//since we're rotating a point, we take on a 1 in the w component for the transformation
		point = vec3(rotationMatrix * vec4(point, 1.f));

		return point;
	}

	glm::vec3 SphereMeshTextured::copyAndRotatePointXAxis(const glm::vec3& toCopy, float rotationDegrees)
	{
		using namespace glm;

		vec3 result(toCopy);

		glm::mat4 rotationMatrix(1.f);
		rotationMatrix = rotate(rotationMatrix, glm::radians(rotationDegrees), vec3(1.f, 0, 0));

		result = vec3(rotationMatrix * vec4(result, 1.f));

		//expecting RVO
		return result;
	}

	void SphereMeshTextured::render()
	{
		ec(glBindVertexArray(vao));
		ec(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)); //renderdoc flagged this draw call as an error without binding an ebo -- I thought the VAO captured the state of ebo, but renderdoc says otherwise. :) 

		ec(glDrawElements(GL_TRIANGLES, triangleElementIndices.size(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0)));
	}

	void SphereMeshTextured::instanceRender(int instanceCount)
	{
		ec(glBindVertexArray(vao));
		ec(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
		ec(glDrawElementsInstanced(GL_TRIANGLES, triangleElementIndices.size(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0), instanceCount));
	}

	void SphereMeshTextured::onAcquireOpenGLResources()
	{
		buildMesh(tolerance);
	}

	void SphereMeshTextured::onReleaseOpenGLResources()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vboPositions);
		glDeleteBuffers(1, &vboNormals);
		glDeleteBuffers(1, &vboTexCoords);
		glDeleteBuffers(1, &ebo);
	}

}

