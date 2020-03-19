#include "SACollisionDebugRenderer.h"
#include "SpaceArcade.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../Rendering/SAWindow.h"
#include "../../../Algorithms/SeparatingAxisTheorem/SATRenderDebugUtils.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../../../Algorithms/SeparatingAxisTheorem/ModelLoader/SATModel.h"
#include "SAPrimitiveShapeRenderer.h"
#include "../GameFramework/SACollisionUtils.h"

namespace SA
{

	/*static*/ bool CollisionDebugRenderer::bRenderCollisionShapes_ui = false;
	/*static*/ bool CollisionDebugRenderer::bRenderCollisionOBB_ui = false;

	void CollisionDebugRenderer::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		WindowSystem& windowSystem = game.getWindowSystem();
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &CollisionDebugRenderer::handleWindowLosingOpenglContext);
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &CollisionDebugRenderer::handleWindowAcquiredOpenglContext);

		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleWindowAcquiredOpenglContext(primaryWindow);
		}

	}

	void CollisionDebugRenderer::handleWindowLosingOpenglContext(const sp<Window>& losingWindow)
	{
		//currently, there is only ever 1 opengl context, but if support multiple simultaneous contexts, the code below
		//will just work. But it also works for our situation (though we could just always set current window to nullptr)
		if (!currentWindow.expired())
		{
			sp<Window> myWindow = currentWindow.lock();
			if (myWindow == losingWindow)
			{
				currentWindow = sp<Window>(nullptr);

				////////////////////////////////////////////////////////
				// Clear out anything with opengl resources!
				////////////////////////////////////////////////////////
				collisionShapeShader = nullptr;
				primitiveShapeRenderer = nullptr;
				capsuleRenderer = nullptr;
			}
		}
	}

	void CollisionDebugRenderer::handleWindowAcquiredOpenglContext(const sp<Window>& acquiredWindow)
	{
		currentWindow = acquiredWindow;
		collisionShapeShader = new_sp<SA::Shader>(SAT::DebugShapeVertSrc, SAT::DebugShapeFragSrc, false);
		primitiveShapeRenderer = new_sp<PrimitiveShapeRenderer>();
		capsuleRenderer = new_sp<SAT::CapsuleRenderer>();
	}

	void CollisionDebugRenderer::renderOBB(const glm::mat4& entityXform, const glm::mat4& aabbLocalXform,
		const glm::mat4& view, const glm::mat4& projection,
		glm::vec3 color, GLenum polygonMode /*= GL_FILL*/, GLenum restorePolygonMode /* = GL_FILL*/)
	{
		ec(glPolygonMode(GL_FRONT_AND_BACK, polygonMode));
		glm::mat4 model = entityXform * aabbLocalXform;

		collisionShapeShader->use();
		collisionShapeShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		collisionShapeShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		collisionShapeShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		collisionShapeShader->setUniform3f("color", color);
		primitiveShapeRenderer->renderUnitCube({ model, view, projection, color, polygonMode, /*will correct after loop*/polygonMode });

		ec(glPolygonMode(GL_FRONT_AND_BACK, restorePolygonMode));

	}

	void CollisionDebugRenderer::renderShape(ECollisionShape shapeType,
		glm::mat4 model, glm::mat4 view, glm::mat4 projection,
		glm::vec3 color, GLenum polygonMode /*= GL_FILL*/, GLenum restorePolygonMode /*= GL_FILL*/)
	{
		if (collisionShapeShader)
		{
			ec(glPolygonMode(GL_FRONT_AND_BACK, polygonMode));

			//#suggested would be a lot better to have matrices combined in MVP and send that to shader, there's not a need to have them separate; but since this is a debug util I don't think it is that big of deal
			collisionShapeShader->use();
			collisionShapeShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			collisionShapeShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			collisionShapeShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			collisionShapeShader->setUniform3f("color", color);


			SpaceArcade& game = SpaceArcade::get();
			static auto renderShape = [](ECollisionShape shape, const sp<Shader>& shader) {
				const sp<const SAT::Model>& modelForShape = SpaceArcade::get().getCollisionShapeFactoryRef().getModelForShape(shape);
				modelForShape->draw(*shader);
			};

			switch (shapeType)
			{
				case ECollisionShape::CUBE:
					{
						//doesn't actually use collision shader above, but other collision shapes will
						primitiveShapeRenderer->renderUnitCube({ model, view, projection, color, polygonMode, /*will correct after loop*/polygonMode });
						break;
					}
				case ECollisionShape::POLYCAPSULE:
					{
						capsuleRenderer->render();
						break;
					}
				case ECollisionShape::WEDGE:
					{
						renderShape(ECollisionShape::WEDGE, collisionShapeShader);
						break;
					}
				case ECollisionShape::PYRAMID:
					{
						renderShape(ECollisionShape::PYRAMID, collisionShapeShader);
						break;
					}
				case ECollisionShape::UVSPHERE:
					{
						renderShape(ECollisionShape::UVSPHERE, collisionShapeShader);
						break;
					}
				case ECollisionShape::ICOSPHERE:
					{
						renderShape(ECollisionShape::ICOSPHERE, collisionShapeShader);
						break;
					}
				default:
					{
						//show a cube if something went wrong
						primitiveShapeRenderer->renderUnitCube({ model, view, projection, color, polygonMode, polygonMode });
						break;
					}
			}
			ec(glPolygonMode(GL_FRONT_AND_BACK, restorePolygonMode));
		}
	}

}

