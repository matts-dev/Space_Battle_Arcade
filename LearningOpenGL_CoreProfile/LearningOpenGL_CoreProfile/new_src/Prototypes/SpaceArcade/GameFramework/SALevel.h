#pragma once
#include "SAGameEntity.h"
#include "Interfaces/SATickable.h"

#include <set>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "RenderModelEntity.h"
#include "SAWorldEntity.h"
#include "mix_ins/CustomGrid_MixIn.h"
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../Tools/DataStructures/MultiDelegate.h"
#include "../Rendering/Lights/SADirectionLight.h"

namespace SA
{
	class LevelSystem;
	class TimeManager;
	struct DirectionLight;

	struct LevelInitializer
	{
		//provide defaults to these arguments
		glm::vec3 worldGridSize{ 16.f };
	};

	/** Base class for a level object */
	class LevelBase : public GameEntity, public Tickable, public RemoveCopies, public RemoveMoves,
		public CustomGrid_MixIn
	{
		friend LevelSystem;
	public:
		LevelBase(const LevelInitializer& init = {});
		virtual ~LevelBase();

	public:
		MultiDelegate<const sp<WorldEntity>&> onSpawnedEntity;
		MultiDelegate<const sp<WorldEntity>&> onUnspawningEntity;

		template<typename T>
		void spawnCompileCheck() { static_assert(std::is_base_of<RenderModelEntity, T>::value, "spawn/unspawn only works with objects that will be rendered."); }

		template<typename T, typename... Args>
		sp<T> spawnEntity(Args&&... args);

		template<typename T>
		bool unspawnEntity(const sp<T>& entity);

		/** Get this level's collision grid */
		inline SH::SpatialHashGrid<WorldEntity>& getWorldGrid() { return worldCollisionGrid; }

		//#SUGGESTED refactor this to just return reference, a level should always have a valid time manager.
		inline const sp<TimeManager>& getWorldTimeManager() { return worldTimeManager; }

		/** returns const to prevent modification; use spawn and unspawn entity to add/remove. 
			#concern this may be an encapsulation issue. Perhaps accessing entities should only be done through the world grid.*/
		const std::set<sp<WorldEntity>>& getWorldEntities() { return worldEntities; }
		const std::vector<DirectionLight>& getDirectionalLights() const { return dirLights; }
		glm::vec3 getAmbientLight() const { return ambientLight; }
	private:
		void startLevel();
		void endLevel();
	protected: //virtuals; protected indicates that sub-classes may need to call super::virtual_method
		virtual void startLevel_v() = 0;
		virtual void endLevel_v() = 0;
		virtual void onEntitySpawned_v(const sp<WorldEntity>& spawned);
		virtual void onEntityUnspawned_v(const sp<WorldEntity>& unspawned);
		virtual bool isLevelActive() { return bLevelActive; }
	protected:
		virtual void tick_v(float dt_sec) {}
	private: //virtuals; private indicates subclasses inherit when function called, but not how function is completed.
		void tick(float dt_sec);
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) {}; //#TODO #replace this with function that takes as parameter render data
	protected: 
		std::set<sp<WorldEntity>> worldEntities; //O(n) walks, but walks will not be very cache friendly as a lot of indirection. 
		std::set<sp<RenderModelEntity>> renderEntities;
		SH::SpatialHashGrid<WorldEntity> worldCollisionGrid;
		sp<TimeManager> worldTimeManager;
		std::vector<DirectionLight> dirLights;
		glm::vec3 ambientLight{0.f};
	private:
		bool bLevelActive = false;
	};

	///////////////////////////////////////////////////////////////////////////////////
	// Template Bodies
	///////////////////////////////////////////////////////////////////////////////////
		template<typename T, typename... Args>
		sp<T> LevelBase::spawnEntity(Args&&... args)
		{
			spawnCompileCheck<T>();
			sp<T> entity = new_sp<T>(std::forward<Args>(args)...);
			worldEntities.insert(entity);
			renderEntities.insert(entity);
			onEntitySpawned_v(entity);
			onSpawnedEntity.broadcast(entity);
			return entity;
		}

		template<typename T>
		bool LevelBase::unspawnEntity(const sp<T>& entity)
		{
			spawnCompileCheck<T>();
			sp<T> tempCopy = entity;

			bool foundInAllLocations = renderEntities.find(entity) != renderEntities.end() && worldEntities.find(entity) != worldEntities.end();
			worldEntities.erase(entity);
			renderEntities.erase(entity);

			onEntityUnspawned_v(entity);
			onUnspawningEntity.broadcast(entity);
			return foundInAllLocations;
		}
}