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

	class ProjectileSystem;
	class ProjectileClassHandle;
	class UISystem;
	
	class UIRootWindow;

	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();

	private:
		virtual sp<SA::Window> startUp() override;
		virtual void shutDown() override;
		virtual void tickGameLoop(float deltaTimeSecs) override;
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
		inline const sp<ProjectileSystem>& getProjectileSys() noexcept { return ProjectileSys; }
		inline const sp<UISystem>& getUISystem() noexcept { return UI_Sys; }
	private:
		sp<ProjectileSystem> ProjectileSys;
		sp<UISystem> UI_Sys;
		/////////////////////////////////////////////////////////////////////////////////////

	public:
		UniformResourceLocators URLs;

	private: //debugging
		void renderDebug(const glm::mat4& view, const glm::mat4& projection);

	public: //debug state (public to allow sub-editor access)
		bool bRenderDebugCells = false;
		bool bRenderProjectileOBBs = false;

	private:
		sp<SA::CameraFPS> fpsCamera;

		//shaders
		sp<SA::Shader> litObjShader;
		sp<SA::Shader> lampObjShader;
		sp<SA::Shader> forwardShaded_EmissiveModelShader;
		sp<SA::Shader> debugLineShader;

		////unit cube data
		//GLuint cubeVAO, cubeVBO;

		sp<Model3D> laserBoltModel;
		sp<ProjectileClassHandle> laserBoltHandle;

		//ui
		sp<UIRootWindow> ui_root;
	};

}