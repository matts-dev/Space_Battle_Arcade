#include "SHDebugUtils.h"

void SH::drawDebugLines(Shader& shader, const glm::vec3& color, const std::vector<glm::vec4>& linesToDraw, const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection)
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
