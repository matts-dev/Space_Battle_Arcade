#pragma once
#include "..\GameFramework\SAGameBase.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <vector>
#include <map>
#include "..\GameFramework\RenderModelEntity.h"
#include "SAUniformResourceLocators.h"


namespace SA
{
	class CameraFPS;
	class Shader;
	class Model3D;

	class ProjectileClassHandle;

	class ProjectileSystem;
	class UISystem;
	class ModSystem;
	
	class CollisionShapeFactory;

	class UIRootWindow;

	class HUD;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The game implementation for the space arcade game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();
	private:
		virtual sp<SA::Window> startUp() override; 
		virtual void shutDown() override;
		virtual void tickGameLoop(float deltaTimeSecs) override;
		virtual void cacheRenderDataForCurrentFrame(struct RenderData& frameRenderData) override;
		virtual void renderLoop(float deltaTimeSecs) override;
		virtual void onRegisterCustomSystem() override;

		void updateInput(float detltaTimeSec);

		/////////////////////////////////////////////////////////////////////////////////////
		//Spawning Interface
		/////////////////////////////////////////////////////////////////////////////////////
	public:

		//////////////////////////////////////////////////////////////////////////////////////
		//  Custom Systems
		/////////////////////////////////////////////////////////////////////////////////////
	public:
		inline const sp<ProjectileSystem>& getProjectileSystem() noexcept { return projectileSystem; }
		inline const sp<UISystem>& getUISystem() noexcept { return uiSystem; }
		inline const sp<ModSystem>& getModSystem() noexcept { return modSystem; }
	private:
		sp<ProjectileSystem> projectileSystem;
		sp<UISystem> uiSystem;
		sp<ModSystem> modSystem;
		/////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////////////
		//  Persistent Storage
		/////////////////////////////////////////////////////////////////////////////////////
	public:
		CollisionShapeFactory& getCollisionShapeFactoryRef() { return *collisionShapeFactory; }
		/** provided for easy caching */
		sp<CollisionShapeFactory> getCollisionShapeFactory() { return collisionShapeFactory; }
	private:
		sp<CollisionShapeFactory> collisionShapeFactory = nullptr;
		////////////////////////////////////////////////////////////////////////////////////

	public:
		UniformResourceLocators URLs;

	private: //debugging
		void renderDebug(const glm::mat4& view, const glm::mat4& projection);

	public: //debug state (public to allow sub-editor access)
		bool bRenderDebugCells = false;
		bool bRenderDebugAvoidanceSphereCells = false;
		bool bRenderProjectileOBBs = false;

	private:
		sp<SA::CameraFPS> fpsCamera;

		//shaders
		sp<Shader> litObjShader;
		sp<Shader> lampObjShader;
		sp<Shader> forwardShaded_EmissiveModelShader;
		sp<Shader> debugLineShader;

		////unit cube data
		//GLuint cubeVAO, cubeVBO;

		//ui
		sp<UIRootWindow> ui_root;
		sp<HUD> hud;
	};

}