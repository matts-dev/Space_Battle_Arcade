#pragma once
#include "SAGameEntity.h"
#include "Interfaces\SATickable.h"

#include <set>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "RenderModelEntity.h"
#include "SAWorldEntity.h"
#include "..\..\..\Algorithms\SpatialHashing\SpatialHashingComponent.h"

namespace SA
{
	class LevelSubsystem;

	/** Base class for a level object */
	class LevelBase : public GameEntity, public Tickable
	{
		friend LevelSubsystem;
	public:
		virtual ~LevelBase();

	public:
		template<typename T>
		void spawnCompileCheck() { static_assert(std::is_base_of<RenderModelEntity, T>::value, "spawn/unspawn only works with objects that will be rendered."); }

		template<typename T, typename... Args>
		sp<T> spawnEntity(Args&&... args);

		template<typename T>
		bool unspawnEntity(const sp<T>& entity);

		/** Get this level's collision grid */
		inline SH::SpatialHashGrid<WorldEntity>& getWorldGrid() { return worldCollisionGrid; }
		float getTimeDialationFactor() { return timeDialationFactor; }

	private:
		void startLevel();
		void endLevel();

	protected: //virtuals; protected indicates that sub-classes may need to call super::virtual_method
		virtual void startLevel_v() = 0;
		virtual void endLevel_v() = 0;
	private: //virtuals; private indicates subclasses inherit when function called, but now how function is completed.
		virtual void tick(float dt_sec);
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) {};

	protected: 
		//O(n) walks, but walks will not be very cache friendly as a lot of indirection. 
		std::set<sp<WorldEntity>> worldEntities;
		std::set<sp<RenderModelEntity>> renderEntities;

		SH::SpatialHashGrid<WorldEntity> worldCollisionGrid{ glm::vec3(4,4,4) };

	private:
		float timeDialationFactor = 1.f;
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
			return entity;
		}

		template<typename T>
		bool LevelBase::unspawnEntity(const sp<T>& entity)
		{
			spawnCompileCheck<T>();
			bool foundInAllLocations = renderEntities.find(entity) != renderEntities.end() && worldEntities.find(entity) != worldEntities.end();
			worldEntities.erase(entity);
			renderEntities.erase(entity);
			return foundInAllLocations;
		}
}