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

	class CollisionSubsystem;
	class ProjectileSubsystem;
	class ProjectileClassHandle;
	class UISubsystem;
	
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
		virtual void onRegisterCustomSubsystem() override;

		void updateInput(float detltaTimeSec);

		/////////////////////////////////////////////////////////////////////////////////////
		//Spawning Interface
		/////////////////////////////////////////////////////////////////////////////////////
	public:

		//////////////////////////////////////////////////////////////////////////////////////
		//  Custom Subsystems
		/////////////////////////////////////////////////////////////////////////////////////
	public:
		inline const sp<ProjectileSubsystem>& getProjectileSS() noexcept { return ProjectileSS; }
		inline const sp<UISubsystem>& getUISubsystem() noexcept { return UI_SS; }
	private:
		sp<ProjectileSubsystem> ProjectileSS;
		sp<UISubsystem> UI_SS;
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