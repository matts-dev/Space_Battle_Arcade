#include "SAPrimitiveShapeRenderer.h"
#include "..\Rendering\OpenGLHelpers.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\GameFramework\SAWindowSubsystem.h"
#include <assert.h>
#include "..\Tools\SAUtilities.h"
#include "..\Rendering\BuiltInShaders.h"
#include "..\Rendering\SAShader.h"


namespace SA
{
	PrimitiveShapeRenderer::~PrimitiveShapeRenderer()
	{
		if (!registeredWindow.expired())
		{
			sp<Window> attachedWindow = registeredWindow.lock();
			handleWindowLosingOpenglContext(attachedWindow);
		}
	}

	void PrimitiveShapeRenderer::renderUnitCube(const RenderParameters& params)
	{
		simpleShader->use();
		simpleShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(params.model));
		simpleShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(params.view));
		simpleShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(params.projection));
		simpleShader->setUniform3f("color", params.color);

		glPolygonMode(GL_FRONT_AND_BACK, params.renderMode);
		renderUnitCube_CustomShader();
		glPolygonMode(GL_FRONT_AND_BACK, params.restoreToRenderMode);

	}

	void PrimitiveShapeRenderer::renderAxes(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::mat4; using glm::vec3;

		float shrinkFactor = 0.01f;
		mat4 xModel = glm::translate(glm::scale(mat4(1.f), { 1, shrinkFactor, shrinkFactor }), glm::vec3(0.5f, 0, 0)) * model;
		mat4 yModel = glm::translate(glm::scale(mat4(1.f), { shrinkFactor, 1, shrinkFactor }), glm::vec3(0, 0.5f, 0)) * model;
		mat4 zModel = glm::translate(glm::scale(mat4(1.f), { shrinkFactor, shrinkFactor, 1 }), glm::vec3(0,0,0.5f)) * model;
		 
		renderUnitCube({ xModel, view, projection, {1,0,0} });
		renderUnitCube({ yModel, view, projection, {0,1,0} });
		renderUnitCube({ zModel, view, projection, {0,0,1} });

	}

	void PrimitiveShapeRenderer::renderUnitCube_CustomShader() const
	{
		ec(glBindVertexArray(cubeVAO));
		ec(glDrawArrays(GL_TRIANGLES, 0, sizeof(Utils::unitCubeVertices_Position_Normal) / (6 * sizeof(float))));
	}

	void PrimitiveShapeRenderer::postConstruct()
	{
		WindowSubsystem& windowSystem = GameBase::get().getWindowSubsystem();

		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &PrimitiveShapeRenderer::handleWindowAquiredOpenglContext);
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &PrimitiveShapeRenderer::handleWindowLosingOpenglContext);

		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleWindowAquiredOpenglContext(primaryWindow);
		}
	}

	void PrimitiveShapeRenderer::handleWindowAquiredOpenglContext(const sp<Window>& window)
	{
		//we shouldn't have a cached window if this is happening
		assert(registeredWindow.expired());

		if (window)
		{
			registeredWindow = window;

			//set up unit cube
			ec(glGenVertexArrays(1, &cubeVAO));
			ec(glBindVertexArray(cubeVAO));
			ec(glGenBuffers(1, &cubeVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(Utils::unitCubeVertices_Position_Normal), Utils::unitCubeVertices_Position_Normal, GL_STATIC_DRAW));
			ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0)));
			ec(glEnableVertexAttribArray(0));
			ec(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))));
			ec(glEnableVertexAttribArray(1));
			ec(glBindVertexArray(0));

			simpleShader = new_sp<Shader>(SA::DebugVertSrc, SA::DebugFragSrc, false);
		}
	}

	void PrimitiveShapeRenderer::handleWindowLosingOpenglContext(const sp<Window>& window)
	{
		if (!registeredWindow.expired())
		{
			sp<Window> attachedWindow = registeredWindow.lock();
			if (attachedWindow == window)
			{
				//clean up OpenGL
				ec(glDeleteVertexArrays(1, &cubeVAO));
				ec(glDeleteBuffers(1, &cubeVBO));

				simpleShader = nullptr;

				registeredWindow = sp<Window>(nullptr);
			}
		}
	}
}
