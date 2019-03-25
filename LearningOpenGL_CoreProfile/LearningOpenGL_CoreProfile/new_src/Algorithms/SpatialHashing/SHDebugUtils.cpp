#include "SHDebugUtils.h"

void SH::drawDebugLines(Shader& shader, const glm::vec3& color, const std::vector<glm::vec4>& linesToDraw, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
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

void SH::drawCells(std::vector<glm::ivec3>& cells, const glm::vec4& gridCellSize, const glm::vec3& color, Shader& debugShader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
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

		//zero corrections
		//if (cell.x == 0)
		//{
		//	startX = gridCellSize.x;
		//	endX = -gridCellSize.x;
		//}
		//if (cell.y == 0)
		//{
		//	startY = gridCellSize.y;
		//	endY = -gridCellSize.y;
		//}
		//if (cell.z == 0)
		//{
		//	startZ = gridCellSize.z;
		//	endZ = -gridCellSize.z;
		//}

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
