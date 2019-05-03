#pragma once
#include "..\GameFramework\SAGameBase.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <vector>


namespace SA
{
	class CameraFPS;
	class Shader;
	class Model3D;

	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();

	private:
		virtual sp<SA::Window> startUp() override;
		virtual void shutDown() override;
		virtual void tickGameLoopDerived(float deltaTimeSecs) override;

		void updateInput(float detltaTimeSec);

	private:
		sp<SA::CameraFPS> fpsCamera;

		//shaders
		sp<SA::Shader> litObjShader;
		sp<SA::Shader> lampObjShader;
		sp<SA::Shader> forwardShadedModelShader;

		//unit cube data
		GLuint cubeVAO, cubeVBO;

		std::vector<sp<Model3D>> loadedModels;
	};
}