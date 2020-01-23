#pragma once
#include "SAGameEntity.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../Tools/DataStructures/MultiDelegate.h"
#include "Interfaces/SATickable.h"
#include "Components/SAComponentEntity.h"

namespace SA
{
	class LevelBase;
	class CollisionInfo;

	/**
		A world entity is a game entity that has a physical presence in the game world.
		Examples:
			*Space ships, *People, *TriggerVolumes, *Bullets, etc

		Features:
			*World entities have a tick method. Under some storage contexts this method may automatically
				be invoked for the user. In other contexts the user may need to register to a callback.
			*World entities define their world transform interface

	*/
	class WorldEntity : public GameplayComponentEntity, public Tickable
	{
	public:
		WorldEntity(Transform spawnTransform = Transform{})
			: transform(spawnTransform)
		{}
		virtual void tick(float deltaTimeSecs) {};

		inline const Transform& getTransform() const noexcept { return transform; }
		virtual void setTransform(const Transform& inTransform);

		//#scenenodes this will need updating to get final parent-child transformed position
		glm::vec3 getWorldPosition() const { return transform.position; } //#scenenodes todo update
		glm::mat4 getModelMatrix() const { return transform.getModelMatrix(); } //#scenenodes todo update

	protected:
		/** World returns a raw pointer because caching a world sp will often result cyclic references. 
			A raw pointer should make a programmer think about how to safely cache it and find this message.*/
		LevelBase* getWorld();
	public:
		MultiDelegate<const Transform& /*xform*/> onTransformUpdated;

	private:
		Transform transform; //#TODO #scenenodes #componentize
	};
}
