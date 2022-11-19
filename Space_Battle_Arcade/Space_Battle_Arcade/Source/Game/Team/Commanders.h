#pragma once

#include <vector>
#include <stack>

#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/LifetimePointer.h"
#include "../../Tools/DataStructures/AdvancedPtrs.h"

namespace SA
{
	class WorldEntity;

	class TeamCommander : public GameplayComponentEntity
	{
	public:
		TeamCommander(size_t team);

		sp<WorldEntity> getTarget();
		sp<WorldEntity> getTargetOnTeam(size_t team);
		bool queueTarget(const sp<WorldEntity>& target);
		void handleCarrierSpawned(const sp<WorldEntity>& target);
		glm::vec3 getCommanderPosition() { return glm::vec3(0, 0, 0); } //#TODO hook up commander positions tied to leader ship
		const std::vector<fwp<WorldEntity>>& getTeamCarriers() { return carriers; }

	protected:

		virtual void postConstruct() override;

	private:
		void handleEntitySpawned(const sp<WorldEntity>& spawned);

	private:
		size_t cachedTeamId;

		//indices represent team number
		std::vector<std::stack<lp<WorldEntity>>> pendingTargetsByTeam;
		std::vector<fwp<WorldEntity>> carriers;

	};

}