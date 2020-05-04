#pragma once
#include "..\GameFramework\SAGameBase.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <vector>
#include <map>
#include "..\GameFramework\RenderModelEntity.h"
#include "SAUniformResourceLocators.h"
#include "OptionalCompilationMacros.h"

namespace SA
{
	class CameraFPS;
	class Shader;
	class Model3D;

	class ProjectileClassHandle;

	class ProjectileSystem;
	class UISystem_Editor;
	class UISystem_Game;
	class ModSystem;
	
	class CollisionShapeFactory;
	class UIRootWindow;
	class HUD;
	class DeveloperConsole;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The game implementation for the space arcade game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();
	private:
		virtual sp<SA::Window> startUp() override; 
		virtual void onShutDown() override;
		virtual void tickGameLoop(float deltaTimeSecs) override;
		virtual void cacheRenderDataForCurrentFrame(struct RenderData& frameRenderData) override;
		virtual void renderLoop(float deltaTimeSecs) override;
		virtual void onRegisterCustomSystem() override;
		virtual sp<CheatSystemBase> createCheatSystemSubclass() override;

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
		inline const sp<UISystem_Editor>& getEditorUISystem() noexcept { return uiSystem_Editor; }
		inline const sp<UISystem_Game>& getGameUISystem() noexcept { return uiSystem_Game; }
		inline const sp<ModSystem>& getModSystem() noexcept { return modSystem; }
	private:
		sp<ProjectileSystem> projectileSystem;
		sp<UISystem_Editor> uiSystem_Editor;
		sp<UISystem_Game> uiSystem_Game;
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
		const sp<HUD> getHUD() const { return hud; }

	public:
		UniformResourceLocators URLs;

	private: //debugging
		void renderDebug(const glm::mat4& view, const glm::mat4& projection);

	public: //debug state (public to allow sub-editor access)
		bool bRenderDebugCells = false;
		bool bRenderDebugAvoidanceSphereCells = false;
		bool bRenderProjectileOBBs = false;
		bool bEnableDevConsoleFeature = true;

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
		sp<DeveloperConsole> console;
	};

}