#pragma once
#include "..\..\GameFramework\SALevel.h"
#include "SABaseLevel.h"

namespace SA
{
	class PlayerBase;
	class Model3D;
	class Mod;
	class SpawnConfig;
	class Shader;

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
		sp<Model3D> renderModel = nullptr;
		sp<SpawnConfig> activeSpawnConfig = nullptr;

		//TODO refactor so that this shader isn't getting re-created for each level/editor
		//it would be nice to have a way to obtain a shared utility shader for these kinds of things
		sp<Shader> model3DShader;

		bool bAutoSave = true;
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;

	};

}