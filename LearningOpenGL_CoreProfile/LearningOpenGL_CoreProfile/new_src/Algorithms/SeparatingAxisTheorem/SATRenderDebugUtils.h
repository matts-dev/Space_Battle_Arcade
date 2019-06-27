#pragma once
#include "SATComponent.h"

#include <glad/glad.h> 
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace SAT
{
	char const* const DebugShapeVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec4 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * position;
				}
			)";
	char const* const DebugShapeFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

	/** This method is very slow and should be avoided in non-testing code */
	template<typename Shader>
	void drawDebugLine(Shader& shader,
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection);

	/** This method is very slow and should be avoided in non-testing code */
	template<typename Shader>
	void drawDebugTriangleFace(Shader& shader,
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& pntC, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection);

	/** This method is very slow and should be avoided in non-testing code */
	template<typename Shader>
	void drawDebugCollisionShape(Shader& shader, const Shape& shape, const glm::vec3& color, float debugPointSize,
		bool bRenderEdges, bool bRenderFaces,
		const glm::mat4& view, const glm::mat4 projection);

	class ShapeRender final
	{
	public:
		ShapeRender(const Shape& shape);
		~ShapeRender();

		//deleted special functions due to VAO/VBO copying complexity
		ShapeRender(const ShapeRender& copy) = delete;
		ShapeRender(ShapeRender&& move) = delete;
		ShapeRender& operator=(const ShapeRender& copy) = delete;
		ShapeRender& operator=(ShapeRender&& move) = delete;

		/** Assumes a pre-configured shader */
		void render(bool bRenderPnts = true, bool bRenderUniqueTris = true);

	private:
		GLuint VAO_pnts;
		GLuint VBO_pnts;
		GLsizei pntCount;

		GLuint VAO_tris;
		GLuint VBO_tris; //only the triangles used in SAT (redundant tris have been culled
		GLuint VBO_normals; //not well tested, relying on non-lighting scenarios -- some normals may point inside model
		GLsizei vertCount;
	};








































	template<typename Shader>
	void drawDebugLine(Shader& shader,
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
	{
		/* This method is extremely slow and non-performant; use only for debugging purposes */
		shader.use();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		shader.setUniform3f("color", color);

		//basically immediate mode, should be very bad performance
		GLuint tmpVAO, tmpVBO;
		glGenVertexArrays(1, &tmpVAO);
		glBindVertexArray(tmpVAO);
		float verts[] = {
			pntA.x,  pntA.y, pntA.z, 1.0f,
			pntB.x, pntB.y, pntB.z, 1.0f
		};
		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_LINES, 0, 2);

		glDeleteVertexArrays(1, &tmpVAO);
		glDeleteBuffers(1, &tmpVBO);
	}

	template<typename Shader>
	void drawDebugTriangleFace(Shader& shader, const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& pntC,
		const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
	{
		/* This method is extremely slow and non-performant; use only for debugging purposes */
		shader.use();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		shader.setUniform3f("color", color);

		//basically immediate mode, should be very bad performance
		GLuint tmpVAO, tmpVBO;
		glGenVertexArrays(1, &tmpVAO);
		glBindVertexArray(tmpVAO);
		float verts[] = {
			pntA.x,  pntA.y, pntA.z, 1.0f,
			pntB.x, pntB.y, pntB.z, 1.0f,
			pntC.x, pntC.y, pntC.z, 1.0f
		};
		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDeleteVertexArrays(1, &tmpVAO);
		glDeleteBuffers(1, &tmpVBO);
	}

	template<typename Shader>
	void drawDebugCollisionShape(Shader& shader, const Shape& shape, const glm::vec3& color, float debugPointSize,
		bool bRenderEdges, bool bRenderFaces,
		const glm::mat4& view, const glm::mat4 projection)
	{
		const std::vector<glm::vec4>& transformedPoints = shape.getTransformedPoints();

		shader.use();
		shader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
		shader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		shader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		shader.setUniform3f("color", color);

		bool pointSizeEnabled = glIsEnabled(GL_POINT_SIZE);
		float originalPointSize = 1.0f;
		glGetFloatv(GL_POINT_SIZE, &originalPointSize);
		if (!pointSizeEnabled)
		{
			glEnable(GL_POINT_SIZE);
			glPointSize(debugPointSize);
		}

		//basically immediate mode, should be very bad performance
		GLuint tmpVAO, tmpVBO;
		glGenVertexArrays(1, &tmpVAO);
		glBindVertexArray(tmpVAO);
		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * transformedPoints.size(), &transformedPoints[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_POINTS, 0, transformedPoints.size());

		glDeleteVertexArrays(1, &tmpVAO);
		glDeleteBuffers(1, &tmpVBO);

		if (!pointSizeEnabled)
		{
			glDisable(GL_POINT_SIZE);
		}
		else
		{
			glPointSize(originalPointSize);
		}

		if (bRenderEdges)
		{
			for (const Shape::EdgePointIndices& edge : shape.getDebugEdgeIdxs())
			{
				drawDebugLine(shader, transformedPoints[edge.indexA], transformedPoints[edge.indexB], color,
					glm::mat4(1.0f), view, projection);
			}
		}
		if (bRenderFaces)
		{
			for (const Shape::FacePointIndices& face : shape.getDebugFaceIdxs())
			{
				uint32_t aIdx = face.edge1.indexA;
				uint32_t bIdx = face.edge1.indexB;
				uint32_t cIdx = face.edge2.indexA;
				cIdx = (cIdx == aIdx || cIdx == bIdx) ? face.edge2.indexB : cIdx;

				drawDebugTriangleFace(shader, transformedPoints[aIdx], transformedPoints[bIdx], transformedPoints[cIdx], color / 2.0f,
					glm::mat4(1.0f), view, projection);
			}
		}
	}

}