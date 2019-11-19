#pragma once

#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include <vector>
#include <stack>
#include "../../Tools/DataStructures/SATransform.h"

namespace SA
{
	class WorldEntity;

	class TeamCommander : public GameplayComponentEntity
	{
	public:
		TeamCommander(size_t team);

		sp<WorldEntity> getTarget();
		sp<WorldEntity> getTargetOnTeam(size_t team);
		glm::vec3 getCommanderPosition() { return glm::vec3(0, 0, 0); } //#TODO hook up commander positions tied to leader ship

	protected:

		virtual void postConstruct() override;

	private:
		void handleEntitySpawned(const sp<WorldEntity>& spawned);

	private:
		size_t cachedTeamId;

		//indices represent team number
		std::vector<std::stack<wp<WorldEntity>>> pendingTargetsByTeam;

	};

}