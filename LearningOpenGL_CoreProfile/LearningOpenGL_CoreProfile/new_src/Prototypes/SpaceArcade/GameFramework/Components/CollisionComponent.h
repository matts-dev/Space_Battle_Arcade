#pragma once
#include "SAComponentEntity.h"


namespace SA
{
	class CollisionInfo;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Wrapper of collision. Encapsulation is tied to constness of component.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CollisionComponent : public GameComponentBase
	{
	public:
		void setCollisionData(const sp<CollisionInfo>& inCollisionData) { collisionData = inCollisionData; }
		CollisionInfo* getCollisionData() { return collisionData.get(); }
		const CollisionInfo* getCollisionData() const { return collisionData.get(); }
		void setKinematicCollision(bool bRequestCollisionTests) { bKinematicCollision = bRequestCollisionTests; }
		bool getKinematicCollision() const { return bKinematicCollision; }
	private:
		/*	Whether this component should try to claim it's space in the 3d world. When false this tells others that this object doesn't mind having spatial overlap.
			But it has collision that can be used for things like projectiles*/
		bool bKinematicCollision = true;		
		sp<CollisionInfo> collisionData;
	};

}
