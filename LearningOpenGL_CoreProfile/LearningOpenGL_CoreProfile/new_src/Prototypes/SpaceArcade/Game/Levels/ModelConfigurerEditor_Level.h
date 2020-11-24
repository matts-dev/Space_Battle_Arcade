#pragma once
#include <map>
#include "SASpaceLevelBase.h"

#include "../AssetConfigs/SASpawnConfig.h"
#include "../../GameFramework/SALevel.h"

namespace SAT
{
	//forward declarations outside of SA namespace
	class CubeShape;
	class PolygonCapsuleShape;
	class ShapeRender;
	class CapsuleRenderer;
	class Shape;
}

namespace SA
{
	class PlayerBase;
	class Model3D;
	class Mod;
	class SpawnConfig;
	class ProjectileConfig;
	class Shader;
	class PrimitiveShapeRenderer;
	class ShapeRenderWrapper;
	class CameraBase;
	class Window;
	class AvoidanceSphere;

	struct ConfigDefaults
	{
		bool bUseModelAABBTest = true;
	};

	class ModelConfigurerEditor_Level : public LevelBase
	{
	public:
		virtual bool isEditorLevel() override { return true; }

	protected: //level base api
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void tick_v(float dt_sec) override;

	private: //event handlers
		void handleUIFrameStarted();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int key, int state, int modifier_keys, int scancode);
		void handleMouseButton(int button, int state, int modifier_keys);
		void handleModChanging(const sp<Mod>& previous, const sp<Mod>& active);
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);

	private: //ui
		void renderUI_LoadingSavingMenu();
		void renderUI_ModModelGlobals();
		void renderUI_Collision();
		void renderUI_Projectiles();
		void renderUI_ViewportUI();
		void renderUI_Team();
		void renderUI_Objectives();
		void renderUI_Spawning();
		void renderUI_Sounds();
		void renderUI_Effects();

	private: 
		/** INVARIANT: filepath has been checked to be a valid model file path */
		void createNewSpawnConfig(const std::string& name, const std::string& fullModelPath);

		void onActiveConfigSet(const SpawnConfig& newConfig);
		void tryLoadCollisionModel(const char* filePath);
		void updateCameras();
		void updateCameraSpeed();
	private: //implementation details
		char engineSoundPathName[4096] = {};
		char projectileSoundPathName[4096] = {};
		char explosionSoundPathName[4096] = {};
		char muzzleSoundPathName[4096] = {};
	private: 
		bool bRenderAABB = true;
		bool bRenderCollisionShapes = true;
		bool bRenderAvoidanceSpheres = true;
		bool bRenderCollisionShapesLines = true;
		bool bShowCustomShapes = false;
		bool bShowSlowShapes = false;
		bool bModelXray = false;
		bool bUseCollisionCamera = false;

		int selectedShapeIdx = -1;
		int selectedAvoidanceSphereIdx = -1;
		int selectedCommPlacementIdx = -1;
		int selectedDefensePlacementIdx = -1;
		int selectedTurretPlacementIdx = -1;
		int selectedSpawnPointIdx = -1;
		int selectedSpawnableConfigNameIdx = -1;
		TeamData activeTeamData;

		float cameraSpeedModifier = 1.f;
	private:
		sp<CameraBase> cachedPlayerCamera = nullptr;
		sp<class CollisionDebugCamera> levelCamera = nullptr;
	private:
		sp<Model3D> renderModel = nullptr;
		sp<SpawnConfig> activeConfig = nullptr;
		sp<ProjectileConfig> primaryProjectileConfig = nullptr;
		std::map<std::string, sp<Model3D>> collisionModels;
		//std::map<std::string, sp<SAT::Shape>> collisionModelSATShapes;
		std::map<std::string, sp<ShapeRenderWrapper>> collisionModelSATShapesRenders; 

		sp<Shader> model3DShader;
		sp<Shader> collisionShapeShader;
		sp<PrimitiveShapeRenderer> shapeRenderer;

		sp<SAT::CubeShape> cubeShape;
		sp<SAT::PolygonCapsuleShape> polyShape;
		sp<SAT::CapsuleRenderer> capsuleRenderer;
		sp<AvoidanceSphere> sharedAvoidanceRenderer;

		//dummy ship placement entities for visuals
		std::vector<sp<ShipPlacementEntity>> placement_communications;
		std::vector<sp<ShipPlacementEntity>> placement_defenses;
		std::vector<sp<ShipPlacementEntity>> placement_turrets;

		bool bAutoSave = true;
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
	};

}