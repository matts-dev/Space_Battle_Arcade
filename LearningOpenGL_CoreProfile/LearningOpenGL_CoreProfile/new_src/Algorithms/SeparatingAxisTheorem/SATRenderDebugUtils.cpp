#include "SATRenderDebugUtils.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>


//void SAT::drawDebugLine(Shader& shader,
//	const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
//	const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
//{
//	/* This method is extremely slow and non-performant; use only for debugging purposes */
//	shader.use();
//	shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
//	shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
//	shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
//	shader.setUniform3f("color", color);
//
//	//basically immediate mode, should be very bad performance
//	GLuint tmpVAO, tmpVBO;
//	glGenVertexArrays(1, &tmpVAO);
//	glBindVertexArray(tmpVAO);
//	float verts[] = {
//		pntA.x,  pntA.y, pntA.z, 1.0f,
//		pntB.x, pntB.y, pntB.z, 1.0f
//	};
//	glGenBuffers(1, &tmpVBO);
//	glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
//	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
//	glEnableVertexAttribArray(0);
//
//	glDrawArrays(GL_LINES, 0, 2);
//
//	glDeleteVertexArrays(1, &tmpVAO);
//	glDeleteBuffers(1, &tmpVBO);
//}
//
//void SAT::drawDebugTriangleFace(Shader& shader, const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& pntC,
//	const glm::vec3& color, 
//	const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
//{
//	/* This method is extremely slow and non-performant; use only for debugging purposes */
//	shader.use();
//	shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
//	shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
//	shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
//	shader.setUniform3f("color", color);
//
//	//basically immediate mode, should be very bad performance
//	GLuint tmpVAO, tmpVBO;
//	glGenVertexArrays(1, &tmpVAO);
//	glBindVertexArray(tmpVAO);
//	float verts[] = {
//		pntA.x,  pntA.y, pntA.z, 1.0f,
//		pntB.x, pntB.y, pntB.z, 1.0f,
//		pntC.x, pntC.y, pntC.z, 1.0f
//	};
//	glGenBuffers(1, &tmpVBO);
//	glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
//	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
//	glEnableVertexAttribArray(0);
//
//	glDrawArrays(GL_TRIANGLES, 0, 3);
//
//	glDeleteVertexArrays(1, &tmpVAO);
//	glDeleteBuffers(1, &tmpVBO);
//}
//
//void SAT::drawDebugCollisionShape(Shader& shader, const Shape& shape, const glm::vec3& color, float debugPointSize,
//									bool bRenderEdges, bool bRenderFaces,
//									const glm::mat4& view, const glm::mat4 projection)
//{
//	const std::vector<glm::vec4>& transformedPoints = shape.getTransformedPoints();
//
//	shader.use();
//	shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
//	shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
//	shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
//	shader.setUniform3f("color", color);
//
//	bool pointSizeEnabled = glIsEnabled(GL_POINT_SIZE);
//	float originalPointSize = 1.0f;
//	glGetFloatv(GL_POINT_SIZE, &originalPointSize);
//	if (!pointSizeEnabled)
//	{
//		glEnable(GL_POINT_SIZE);
//		glPointSize(debugPointSize);
//	}
//
//	//basically immediate mode, should be very bad performance
//	GLuint tmpVAO, tmpVBO;
//	glGenVertexArrays(1, &tmpVAO);
//	glBindVertexArray(tmpVAO);
//	glGenBuffers(1, &tmpVBO);
//	glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * transformedPoints.size(), &transformedPoints[0], GL_STATIC_DRAW);
//	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
//	glEnableVertexAttribArray(0);
//
//	glDrawArrays(GL_POINTS, 0, transformedPoints.size());
//
//	glDeleteVertexArrays(1, &tmpVAO);
//	glDeleteBuffers(1, &tmpVBO);
//
//	if (!pointSizeEnabled)
//	{
//		glDisable(GL_POINT_SIZE);
//	}
//	else
//	{
//		glPointSize(originalPointSize);
//	}
//
//	if (bRenderEdges)
//	{
//		for (const Shape::EdgePointIndices& edge : shape.getDebugEdgeIdxs())
//		{
//			drawDebugLine(shader, transformedPoints[edge.indexA], transformedPoints[edge.indexB], color,
//				glm::mat4(1.0f), view, projection);
//		}
//	}
//	if (bRenderFaces)
//	{
//		for (const Shape::FacePointIndices& face : shape.getDebugFaceIdxs())
//		{
//			uint32_t aIdx = face.edge1.indexA;
//			uint32_t bIdx = face.edge1.indexB;
//			uint32_t cIdx = face.edge2.indexA;
//			cIdx = (cIdx == aIdx || cIdx == bIdx) ? face.edge2.indexB : cIdx;
//
//			drawDebugTriangleFace(shader, transformedPoints[aIdx], transformedPoints[bIdx], transformedPoints[cIdx], color/2.0f,
//				glm::mat4(1.0f), view, projection);
//		}
//	}
//
//
//}

SAT::ShapeRender::ShapeRender(const Shape& shape)
{
	using glm::vec3; using glm::vec4; using glm::mat4;

	const std::vector<glm::vec4>& localPoints = shape.getLocalPoints();
	std::vector<glm::vec4> triPnts;
	std::vector<glm::vec3> triNorms;
	
	//generate triangles for every face
	for (const Shape::FacePointIndices& face : shape.getDebugFaceIdxs())
	{
		//not sure, but this may not respect CCW ordering for some shapes
		uint32_t aIdx = face.edge1.indexA;
		uint32_t bIdx = face.edge1.indexB;
		uint32_t cIdx = face.edge2.indexA;
		cIdx = (cIdx == aIdx || cIdx == bIdx) ? face.edge2.indexB : cIdx;
		
		triPnts.push_back(localPoints[aIdx]);
		triPnts.push_back(localPoints[bIdx]);
		triPnts.push_back(localPoints[cIdx]);

		//normal generation is not well tested, and if triPnts are not in CCW ordering, the normals will point internally
		glm::vec3 first = glm::vec3(localPoints[bIdx] - localPoints[aIdx]);
		glm::vec3 second = glm::vec3(localPoints[cIdx] - localPoints[aIdx]);
		glm::vec4 normal = glm::vec4(glm::normalize(glm::cross(first, second)), 0.f);
		triNorms.push_back(normal);
		triNorms.push_back(normal);
		triNorms.push_back(normal);
	}
	vertCount = triPnts.size();

	glGenVertexArrays(1, &VAO_tris);
	glBindVertexArray(VAO_tris);

	glGenBuffers(1, &VBO_tris);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_tris);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * triPnts.size(), &triPnts[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &VBO_normals);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * triNorms.size(), &triNorms[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(1));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	//POINTS
	glGenVertexArrays(1, &VAO_pnts);
	glBindVertexArray(VAO_pnts);
	glGenBuffers(1, &VBO_pnts);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_pnts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * localPoints.size(), &localPoints[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	pntCount = localPoints.size();
	glBindVertexArray(0);
}

SAT::ShapeRender::~ShapeRender()
{
	glDeleteVertexArrays(1, &VAO_tris);
	glDeleteVertexArrays(1, &VAO_pnts);

	glDeleteBuffers(1, &VBO_tris);
	glDeleteBuffers(1, &VBO_normals);
	glDeleteBuffers(1, &VBO_pnts);
}

void SAT::ShapeRender::render(bool bRenderPnts /*= true*/, bool bRenderUniqueTris /*= true*/)
{
	if (bRenderUniqueTris)
	{
		glBindVertexArray(VAO_tris);
		glDrawArrays(GL_TRIANGLES, 0, vertCount);
		glBindVertexArray(0);
	}

	if (bRenderPnts)
	{
		bool pointSizeEnabled = glIsEnabled(GL_POINT_SIZE);
		float originalPointSize = 1.0f;
		glGetFloatv(GL_POINT_SIZE, &originalPointSize);
		if (!pointSizeEnabled)
		{
			glEnable(GL_POINT_SIZE);
			glPointSize(5.f);
		}

		glBindVertexArray(VAO_pnts);
		glDrawArrays(GL_POINTS, 0, pntCount);

		glPointSize(originalPointSize);
		if (!pointSizeEnabled)
		{
			glDisable(GL_POINT_SIZE);
		}
		glBindVertexArray(0);
	}
}
