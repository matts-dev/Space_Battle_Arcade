#include "NdcQuad.h"
#include "OpenGLHelpers.h"
#include <glad/glad.h>



namespace SA
{
	void NdcQuad::render()
	{
		ec(glBindVertexArray(quad_VAO));
		ec(glDrawArrays(GL_TRIANGLES, 0, numVerts));
	}

	void NdcQuad::onAcquireGPUResources()
	{
		//create a render quad
		float quadVertices[] = {
			//x,y,z         s,t
			-1, -1, 0,      0, 0,
			 1, -1, 0,      1, 0,
			 1,  1, 0,      1, 1,

			-1, -1, 0,      0, 0,
			 1,  1, 0,      1, 1,
			-1,  1, 0,      0, 1
		};
		numVerts = (sizeof(quadVertices) / sizeof(float)) / 5;
		
		ec(glGenVertexArrays(1, &quad_VAO));
		ec(glBindVertexArray(quad_VAO));
		
		ec(glGenBuffers(1, &quad_VBO));
		ec(glBindBuffer(GL_ARRAY_BUFFER, quad_VBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));

		ec(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))));
		ec(glEnableVertexAttribArray(1));

		ec(glBindVertexArray(0));
	}

	void NdcQuad::onReleaseGPUResources()
	{
		ec(glDeleteVertexArrays(1, &quad_VAO));
		ec(glDeleteBuffers(1, &quad_VBO));
	}
}


