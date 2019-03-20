#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "..\..\..\Shader.h"
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

	void drawDebugLines(Shader& shader,
		const glm::vec3& color, const std::vector<glm::vec4>& linesToDraw,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection
		);


	template<typename T>
	void drawAABBGrid(const SH::SpatialHashGrid<T>& spatialHash, const glm::vec3& gridColor, Shader& debugGridShader,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
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