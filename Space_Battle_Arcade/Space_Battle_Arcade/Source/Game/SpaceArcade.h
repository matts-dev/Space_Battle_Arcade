#pragma once
#include "GameFramework/SAGameBase.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <vector>
#include <map>
#include "GameFramework/RenderModelEntity.h"
#include "SAUniformResourceLocators.h"
#include "Game/OptionalCompilationMacros.h"
#include "Rendering/CustomGameShaders.h"
#include "GameFramework/EngineCompileTimeFlagsAndMacros.h"

namespace SA
{
	struct SATickGroups;
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
	class PlayerPilotAssistUI;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The game implementation for the space arcade game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();
	private:
		virtual sp<Window> makeInitialWindow() override;
		virtual void startUp() override; 
		virtual void onShutDown() override;
		virtual void tickGameLoop(float deltaTimeSecs) override;
		virtual void cacheRenderDataForCurrentFrame(struct RenderData& frameRenderData) override;
		virtual void renderLoop_begin(float deltaTimeSecs) override;
		virtual void renderLoop_end(float deltaTimeSecs) override;
		virtual void onRegisterCustomSystem() override;
		virtual sp<CheatSystemBase> createCheatSystemSubclass() override;
		virtual sp<TickGroups> onRegisterTickGroups();

		void updateInput(float detltaTimeSec);

		//#todo this should probably be done somewhere else, eg a subclass of level system, but doing this now to finish the game befoer the new year
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);

		/////////////////////////////////////////////////////////////////////////////////////
		// Delegates
		/////////////////////////////////////////////////////////////////////////////////////
	public:
		MultiDelegate<> onForwardToneMappingComplete; //broadcasts after tonemapping when using forward shading

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
		void toggleEditorUIMainMenuVisible();
		bool isEditorMainMenuOnScreen() const;
		bool isEditorMainmenuFeatureEnabled() const { return bEditorMainMenuEnabled; }
		bool escapeShouldOpenEditorMenu() const { return bEscapeShouldOpenEditorMenu; }
		void setEscapeShouldOpenEditorMenu(bool bValue);
		void setEditorMainmenuFeatureEnabled(bool bEnabled) { bEditorMainMenuEnabled = bEnabled; }
		void setClearColor(glm::vec3 inClearColor);
		glm::vec3 getClearColor() const { return renderClearColor; }
		const CustomGameShaders& getGameCustomShaders() const {return customShaders; }
		CustomGameShaders& getGameCustomShaders(){ return customShaders; }
	private:
		bool bEditorMainMenuEnabled = true;
		bool bEscapeShouldOpenEditorMenu = true & !SHIPPING_BUILD;
	public:
		const sp<HUD> getHUD() const { return hud; }
	public:
	public:
		UniformResourceLocators URLs;

	private: //debugging
		void renderDebug(const glm::mat4& view, const glm::mat4& projection);

	public: //debug state (public to allow sub-editor access)
		bool bRenderDebugCells = false;
		bool bRenderDebugAvoidanceSphereCells = false;
		bool bRenderProjectileOBBs = false;
		bool bEnableDevConsoleFeature = true;
		bool bEnableDebugEngineKeybinds = true && !SHIPPING_BUILD;
	public: //subclass engine config variables
		bool bEnableStencilHighlights = true;
		bool bOnlyHighlightTargets = true;


	////////////////////////////////////////////////////////
	// custom tick groups
	////////////////////////////////////////////////////////
	public:
		inline const SATickGroups& saTickGroups() { return *_SATickGroups; }
	private:
		sp<SATickGroups> _SATickGroups = nullptr;

	////////////////////////////////////////////////////////
	// other
	////////////////////////////////////////////////////////

	private:
		sp<SA::CameraFPS> fpsCamera;

		//shaders
		sp<Shader> litObjShader;
		sp<Shader> lampObjShader;
		//sp<Shader> forwardShaded_EmissiveModelShader;
		sp<Shader> debugLineShader;
		CustomGameShaders customShaders;

		glm::vec3 renderClearColor{ 0.f };

		//ui
		sp<UIRootWindow> ui_root_editor;
		sp<HUD> hud; //#TODO would like to move this to player class, but it will become very easy for circular references to become a thing, the HUDs needs to live external to player but be associated with player
		sp<DeveloperConsole> console;
		sp<PlayerPilotAssistUI> playerPilotAssistUI;
	};

}