#pragma once
#include "GameFramework/SAGameEntity.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "Tools/DataStructures/SATransform.h"

namespace SA
{
	class Window;
	class Shader;

	////////////////////////////////////////////////////////////////////////////////
	// Simple shape renderer that can be used to render debug shapes, or shapes
	// using custom shaders.
	///////////////////////////////////////////////////////////////////////////////
	class PrimitiveShapeRenderer : public GameEntity
	{
		////////////////////////////////////////////////////////////////////////////////
		// Customizable Render Parameters
		///////////////////////////////////////////////////////////////////////////////
		public:
			struct RenderParameters
			{
				const glm::mat4& model;
				const glm::mat4& view;
				const glm::mat4& projection;
				const glm::vec3 color;
				const GLenum renderMode = GL_FILL;
				const GLenum restoreToRenderMode = GL_FILL;
			};

	public:
		virtual ~PrimitiveShapeRenderer();

		void renderUnitCube(const RenderParameters& params);
		void renderAxes(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);

		/** These render functions expect you to have pre-configured a shader with uniforms and that there is a registered window*/
		void renderUnitCube_CustomShader() const;

	private:
		virtual void postConstruct() override;

		void handleWindowAquiredOpenglContext(const sp<Window>& window);
		void handleWindowLosingOpenglContext(const sp<Window>& window);


	private:
		wp<Window> registeredWindow;

		sp<Shader> simpleShader;

		//unit cube data
		GLuint cubeVAO, cubeVBO;
	};
}