#pragma once
#include "SAGameEntity.h"
#include "..\Tools\DataStructures\SATransform.h"
#include "Interfaces\SATickable.h"

namespace SA
{
	class LevelBase;
	class ModelCollisionInfo;

	/**
		A world entity is a game entity that has a physical presence in the game world.
		Examples:
			*Space ships, *People, *TriggerVolumes, *Bullets, etc

		Features:
			*World entities have a tick method. Under some storage contexts this method may automatically
				be invoked for the user. In other contexts the user may need to register to a callback.
			*World entities define their world transform interface

	*/
	class WorldEntity : public GameEntity, public Tickable
	{
	public:
		WorldEntity(Transform spawnTransform = Transform{})
			: transform(spawnTransform)
		{}
		virtual void tick(float deltaTimeSecs) {};

		inline const Transform& getTransform() const noexcept				{ return transform; }
		inline void				setTransform(const Transform& inTransform)	{ transform = inTransform; }

		//#scenenodes this will need updating to get final parent-child transformed position
		glm::vec3 getWorldPosition() { return transform.position; } //#scenenodes todo update
		glm::mat4 getModelMatrix() { return transform.getModelMatrix(); } //#scenenodes todo update

		/* returns reference for speed, to opt out of containing collision data, override hasCollisionData to false and
		 use the default implementation WorldEntity::getCollisionInfo() to return nullptr;*/
		virtual const sp<const ModelCollisionInfo>& getCollisionInfo() const = 0;
		virtual bool hasCollisionInfo() const = 0;

	protected:
		/** World returns a raw pointer because caching a world sp will often result cyclic references. 
			A raw pointer should make a programmer think about how to safely cache it and find this message.*/
		LevelBase* getWorld();

	private:
		Transform transform;
	};
}
