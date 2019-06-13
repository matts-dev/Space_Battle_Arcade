#pragma once
#include "SAGameEntity.h"
#include "..\Tools\DataStructures\SATransform.h"
#include "Interfaces\SATickable.h"

namespace SA
{
	class Level;

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

	protected:
		/** World returns a raw pointer because caching a world sp will often result cyclic references. 
			A raw pointer should make a programmer think about how to safely cache it and find this message.*/
		Level* getWorld();

	private:
		Transform transform;
	};
}
