#include "TickGroupManager.h"
#include "../SAGameBase.h"
#include "../SALog.h"
#include "../../Tools/PlatformUtils.h"
#include "../../Game/OptionalCompilationMacros.h"
#include <algorithm>

namespace SA
{
	const SA::TickGroups& TickGroups::get()
	{
#ifdef DEBUG_BUILD
		//tick groups must be ready when this gets called, otherwise we can get into undefined behavior. This happens very early on and it is likely
		//an engine issue if tick groups are not ready when this is called.
		assert(GameBase::get().getTickGroupManager().areTickGroupsInitialized()); 
#endif

		//convenience function; subclasses should provide a similar method. This method uses the Game as 
		//a memory location for unit test reasons. Single can be swapped out for unit tests.
		// subclasses should static_cast the GameBseTickGroup or cache it locally at creation, since there should only ever be 1 instance of a tickgroup created.
		return GameBase::get().tickGroups();
	}


	TickGroupDefinition::TickGroupDefinition(const std::string& inGroupName, float inPriority) :name(inGroupName), priority(inPriority)
	{
		if (GameBase::get().getTickGroupManager().registerTickGroup(*this, TickGroupManager::TickGroupKey{}))
		{
			bRegistered = true;
		}
		else
		{
			//this is being constructed at an incorrect time, did you accidentially pass by value?
			STOP_DEBUGGER_HERE();
		}
	}


	bool TickGroupManager::registerTickGroup(const TickGroupDefinition& tickgroup, const TickGroupKey& privateKey)
	{
		if (bTickgroupRegistrationActive)
		{
			//do custom set up logic (like assigning GUIDs, etc)
			//this may not even be needed as we may can use object addresses
			groupsToSort.push_back(&tickgroup); //address is safe if this is correctly using a global table of tick group declarations

			return true;
		}
		else
		{
			//tick groups are a static concept, they are initialized during engine startup. They should not be created after that.
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to create a new tickgroup outside tickgroup initialization window. Did you forget to pass by reference?");
			STOP_DEBUGGER_HERE();
		}
		return false;
	}

	void TickGroupManager::start_TickGroupRegistration(const GameBaseKey& privateKey)
	{
		bTickgroupRegistrationActive = true;
	}

	void TickGroupManager::stop_TickGroupRegistration(const GameBaseKey& privateKey)
	{
		localGroupCopies.clear();

		//sort the tick groups based on priority
		std::sort(groupsToSort.begin(), groupsToSort.end(), [](const TickGroupDefinition* a, const TickGroupDefinition* b) 
		{
			return a->priority < b->priority;
		});

		//update the table's sort indices so these can be used for comparisons
		for (size_t idx = 0; idx < groupsToSort.size(); ++idx)
		{
			//update the tickgroup definition, this allows us to bypass any group name string comparisions and use the index in the global table.
			groupsToSort[idx]->_sortIdx = idx;

			//copy the data over so that no lifetime issues are possible during engine shutdown. These copies can be used to instantiate level tick groups
			localGroupCopies.push_back(*groupsToSort[idx]);
		}

		groupsToSort.clear();

		bTickgroupRegistrationActive = false;
		bTickGroupsInitialized = true;
	}


}

