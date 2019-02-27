#pragma once
#include "SATComponent.h"
#include "..\..\..\Shader.h"

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
	void drawDebugLine(Shader& shader,
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection);

	/** This method is very slow and should be avoided in non-testing code */
	void drawDebugTriangleFace(Shader& shader,
		const glm::vec3& pntA, const glm::vec3& pntB, const glm::vec3& pntC, const glm::vec3& color,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4 projection);

	/** This method is very slow and should be avoided in non-testing code */
	void drawDebugCollisionShape(Shader& shader, const Shape& shape, const glm::vec3& color, float debugPointSize,
									bool bRenderEdges, bool bRenderFaces,
									const glm::mat4& view, const glm::mat4 projection);
}


