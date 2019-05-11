#pragma once
#include "..\GameFramework\SAGameBase.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <vector>
#include <map>
#include "..\GameFramework\RenderModelEntity.h"


namespace SA
{
	class CameraFPS;
	class Shader;
	class Model3D;
	class CollisionSubsystem;

	class SpaceArcade : public GameBase
	{
	public:
		static SpaceArcade& get();

	private:
		virtual sp<SA::Window> startUp() override;
		virtual void shutDown() override;
		virtual void tickGameLoop(float deltaTimeSecs) override;
		virtual void onRegisterCustomSubsystem() override;

		void updateInput(float detltaTimeSec);

		/////////////////////////////////////////////////////////////////////////////////////
		//Spawning Interface
		/////////////////////////////////////////////////////////////////////////////////////
	private:
		// all are #candidate for gamebase
		template<typename T>
		void spawnCompileCheck(){static_assert(std::is_base_of<RenderModelEntity, T>::value, "spawn/unspawn only works with objects that will be rendered.");}

		template<typename T, typename... Args>
		sp<T> spawnEntity(Args&&... args);

		template<typename T>
		bool unspawnEntity(const sp<T>& entity);

		//////////////////////////////////////////////////////////////////////////////////////
		//  Custom Subsystems
		/////////////////////////////////////////////////////////////////////////////////////
	public:
		inline const sp<CollisionSubsystem>& getCollisionSS() noexcept { return CollisionSS; }
	private:
		sp<CollisionSubsystem> CollisionSS;
		/////////////////////////////////////////////////////////////////////////////////////

	private: //debugging
		void renderDebug(const glm::mat4& view, const glm::mat4& projection);
		bool bRenderDebugCells = false;

	private:
		sp<SA::CameraFPS> fpsCamera;

		//shaders
		sp<SA::Shader> litObjShader;
		sp<SA::Shader> lampObjShader;
		sp<SA::Shader> forwardShadedModelShader;

		//unit cube data
		GLuint cubeVAO, cubeVBO;

		std::vector<sp<Model3D>> loadedModels;

		//gameplay
		std::set<sp<WorldEntity>> worldEntities;
		std::set<sp<RenderModelEntity>> renderEntities;
	};


	///////////////////////////////////////////////////////////////////////////////////
	// Template Bodies
	///////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename... Args>
	sp<T> SpaceArcade::spawnEntity(Args&&... args)
	{
		spawnCompileCheck<T>();
		sp<T> entity = new_sp<T>(std::forward<Args>(args)...);
		worldEntities.insert(entity);
		renderEntities.insert(entity);
		return entity;
	}

	template<typename T>
	bool SpaceArcade::unspawnEntity(const sp<T>& entity)
	{
		spawnCompileCheck<T>();
		bool foundInAllLocations = renderEntities.find(entity) != renderEntities.end() && worldEntities.find(entity) != worldEntities.end();
		worldEntities.erase(entity);
		renderEntities.erase(entity);
		return foundInAllLocations;
	}
}