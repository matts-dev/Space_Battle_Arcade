#pragma once

#include <glad/glad.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "../GameFramework/SAGameEntity.h"

namespace SAT
{
	//forward declarations
	class CapsuleRenderer;
}

namespace SA
{
	//forward declarations
	enum class ECollisionShape : int;
	class Shader;
	class Window;
	class PrimitiveShapeRenderer;

	class CollisionDebugRenderer : public GameEntity
	{
	public:
		static bool bRenderCollisionShapes_ui;
		static bool bRenderCollisionOBB_ui;
	private:
		virtual void postConstruct() override;

		void handleWindowLosingOpenglContext(const sp<Window>& losingWindow);
		void handleWindowAcquiredOpenglContext(const sp<Window>& acquiredWindow);

	public:
		void renderOBB(const glm::mat4& entityXform, const glm::mat4& aabbLocalXform, 
			const glm::mat4& view, const glm::mat4& projection,
			glm::vec3 color, GLenum polygonMode = GL_FILL, GLenum restorePolygonMode = GL_FILL);

		void renderShape(ECollisionShape shape, 
			glm::mat4 model, glm::mat4 view, glm::mat4 projection,
			glm::vec3 color, GLenum polygonMode = GL_FILL, GLenum restorePolygonMode = GL_FILL);
	private:
		wp<Window> currentWindow;

		sp<Shader> collisionShapeShader;
		sp<PrimitiveShapeRenderer> primitiveShapeRenderer;
		sp<SAT::CapsuleRenderer> capsuleRenderer;
	};
}
