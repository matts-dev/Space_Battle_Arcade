#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "../../../Shader.h"
#include "SpatialHashingComponent.h"

namespace SH
{
	char const* const DebugLinesVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec4 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * position;
				}
			)";
	char const* const DebugLinesFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

	template<typename Shader>
	inline void drawDebugLines(Shader& shader, const glm::vec3& color, const std::vector<glm::vec4>& linesToDraw, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
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

		glGenBuffers(1, &tmpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * linesToDraw.size(), &linesToDraw[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_LINES, 0, linesToDraw.size());

		glDeleteVertexArrays(1, &tmpVAO);
		glDeleteBuffers(1, &tmpVBO);
	}

	template <typename Shader>
	inline void drawCells(std::vector<glm::ivec3>& cells, const glm::vec3& gridCellSize, const glm::vec3& color, Shader& debugShader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
	{
		if (cells.size() == 0) return;

		std::vector<glm::vec4> linesToDraw;
		for (glm::ivec3& cell : cells)
		{
			float startX = (cell.x * gridCellSize.x);
			float startY = (cell.y * gridCellSize.y);
			float startZ = (cell.z * gridCellSize.z);

			float endX = startX + gridCellSize.x;
			float endY = startY + gridCellSize.y;
			float endZ = startZ + gridCellSize.z;

			linesToDraw.push_back({ startX, startY, startZ, 1.0f });
			linesToDraw.push_back({ startX, startY, endZ, 1.0f });
			linesToDraw.push_back({ startX, endY, startZ, 1.0f });
			linesToDraw.push_back({ startX, endY, endZ, 1.0f });
			linesToDraw.push_back({ endX, startY, startZ, 1.0f });
			linesToDraw.push_back({ endX, startY, endZ, 1.0f });
			linesToDraw.push_back({ endX, endY, startZ, 1.0f });
			linesToDraw.push_back({ endX, endY, endZ, 1.0f });

			linesToDraw.push_back({ startX, startY, startZ, 1.0f });
			linesToDraw.push_back({ endX, startY, startZ, 1.0f });
			linesToDraw.push_back({ startX, endY, startZ, 1.0f });
			linesToDraw.push_back({ endX,   endY, startZ, 1.0f });
			linesToDraw.push_back({ startX, startY, endZ, 1.0f });
			linesToDraw.push_back({ endX, startY, endZ, 1.0f });
			linesToDraw.push_back({ startX, endY, endZ, 1.0f });
			linesToDraw.push_back({ endX,   endY, endZ, 1.0f });

			linesToDraw.push_back({ startX, startY, startZ, 1.0f });
			linesToDraw.push_back({ startX, endY, startZ, 1.0f });
			linesToDraw.push_back({ startX, startY, endZ, 1.0f });
			linesToDraw.push_back({ startX, endY, endZ, 1.0f });
			linesToDraw.push_back({ endX, startY, startZ, 1.0f });
			linesToDraw.push_back({ endX, endY, startZ, 1.0f });
			linesToDraw.push_back({ endX, startY, endZ, 1.0f });
			linesToDraw.push_back({ endX, endY, endZ, 1.0f });
		}
		drawDebugLines(debugShader, color, linesToDraw, model, view, projection);
	}

	template<typename T, typename Shader>
	void drawAABBGrid(const SH::SpatialHashGrid<T>& spatialHash, const glm::vec3& gridColor, Shader& debugGridShader,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
	{
		static std::vector<glm::vec4> linesToDraw;
		linesToDraw.reserve(1000);
		linesToDraw.clear();
		glm::vec4 startCorner(glm::vec3(-10.f), 1.0f);
		glm::vec4 endCorner(glm::vec3(-startCorner), 1.0f);

		//xy sweep
		float lastLineExtra = 0.1f;
		glm::vec4 xSweep = startCorner;
		while (xSweep.x < endCorner.x + lastLineExtra)
		{
			glm::vec4 ySweep = xSweep;
			while (ySweep.y < endCorner.y + lastLineExtra)
			{
				glm::vec4 startPnt = ySweep;
				glm::vec4 endPnt = ySweep;
				endPnt.z *= -1;

				linesToDraw.push_back(startPnt);
				linesToDraw.push_back(endPnt);
				ySweep.y += spatialHash.gridCellSize.y;
			}
			xSweep.x += spatialHash.gridCellSize.x;
		}

		//xz sweep
		xSweep = startCorner;
		while (xSweep.x < endCorner.x + lastLineExtra)
		{
			glm::vec4 zSweep = xSweep;
			while (zSweep.z < endCorner.z + lastLineExtra)
			{
				glm::vec4 startPnt = zSweep;
				glm::vec4 endPnt = zSweep;
				endPnt.y *= -1;

				linesToDraw.push_back(startPnt);
				linesToDraw.push_back(endPnt);
				zSweep.z += spatialHash.gridCellSize.z;
			}
			xSweep.x += spatialHash.gridCellSize.x;
		}

		//todo: zy sweep
		glm::vec4 zSweep = startCorner;
		while (zSweep.z < endCorner.z + lastLineExtra)
		{
			glm::vec4 ySweep = zSweep;
			while (ySweep.y < endCorner.y + lastLineExtra)
			{
				glm::vec4 startPnt = ySweep;
				glm::vec4 endPnt = ySweep;
				endPnt.x *= -1;

				linesToDraw.push_back(startPnt);
				linesToDraw.push_back(endPnt);
				ySweep.y += spatialHash.gridCellSize.y;
			}
			zSweep.z += spatialHash.gridCellSize.z;
		}

		drawDebugLines(debugGridShader, gridColor, linesToDraw,
			glm::mat4(1.0f), view, projection
		);
	}
}