#pragma once
#include "..\..\GameFramework\SALevel.h"
#include "SASpaceLevelBase.h"

namespace SAT
{
	//forward declarations outside of SA namespace
	class CubeShape;
	class PolygonCapsuleShape;
	class ShapeRender;
	class CapsuleRenderer;
}

namespace SA
{
	class PlayerBase;
	class Model3D;
	class Mod;
	class SpawnConfig;
	class Shader;
	class PrimitiveShapeRenderer;

	struct ConfigDefaults
	{
		bool bUseModelAABBTest = true;
	};

	class ModelConfigurerEditor_Level : public LevelBase
	{
	protected: //level base api
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

	private: //event handlers
		void handleUIFrameStarted();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int key, int state, int modifier_keys, int scancode);
		void handleMouseButton(int button, int state, int modifier_keys);
		void handleModChanging(const sp<Mod>& previous, const sp<Mod>& active);

	private: //ui
		void renderUI_LoadingSavingMenu();
		void renderUI_Collision();
		void renderUI_Projectiles();
		void renderUI_Material();
		void renderUI_Team();

	private: 
		/** INVARIANT: filepath has been checked to be a valid model file path */
		void createNewSpawnConfig(const std::string& name, const std::string& fullModelPath);

	private: 
		bool bRenderAABB = true;
		bool bRenderCollisionShapes = true;
		bool bRenderCollisionShapesLines = false;
		bool bShowCustomShapes = false;
		bool bShowSlowShapes = false;
		int selectedShapeIdx = -1;

	private:
		sp<Model3D> renderModel = nullptr;
		sp<SpawnConfig> activeConfig = nullptr;

		sp<Shader> model3DShader;
		sp<Shader> collisionShapeShader;
		sp<PrimitiveShapeRenderer> shapeRenderer;

		sp<SAT::CubeShape> cubeShape;
		sp<SAT::PolygonCapsuleShape> polyShape;
		sp<SAT::CapsuleRenderer> capsuleRenderer;

		bool bAutoSave = true;
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;

	};

}