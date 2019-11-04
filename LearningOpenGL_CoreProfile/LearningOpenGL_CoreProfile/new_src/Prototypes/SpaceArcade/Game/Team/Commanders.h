#pragma once

#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include <vector>
#include <stack>

namespace SA
{
	class WorldEntity;

	class TeamCommander : public GameplayComponentEntity
	{
	public:
		TeamCommander(size_t team);

		sp<WorldEntity> getTarget();
		sp<WorldEntity> getTargetOnTeam(size_t team);

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