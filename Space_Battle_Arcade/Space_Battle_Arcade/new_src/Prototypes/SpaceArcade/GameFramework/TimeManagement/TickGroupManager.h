#pragma once
#include <string>
#include <vector>

namespace SA
{
	/////////////////////////////////////////////////////////////////////////////////////
	// Represents a named group that is ticked together. Priority determines tick order 
	//  relative to other tick groups. This should be treated as plain old data.
	/////////////////////////////////////////////////////////////////////////////////////
	struct TickGroupDefinition final
	{
	public:
		TickGroupDefinition(const std::string& inGroupName, float inPriority);
		bool isRegistered() const{ return bRegistered; } //registration sorts tick groups up front so that array only needs to be sorted once.
	public:
		std::string name;
		float priority;
		inline size_t sortIdx() const { return _sortIdx; }
	private:
		friend class TickGroupManager;
		mutable size_t _sortIdx = 0;	//mutable so TickGroupmanager can modify constant data in the table of tick group declarations
		mutable bool bRegistered = false;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	//Subclass this to add new tickgroups in a single location.
	/////////////////////////////////////////////////////////////////////////////////////
	struct TickGroups
	{
		const TickGroupDefinition GAME					= TickGroupDefinition("GAME", 25.f);
		const TickGroupDefinition PRE_CAMERA			= TickGroupDefinition("PRE_CAMERA", 70.f);
		const TickGroupDefinition CAMERA				= TickGroupDefinition("CAMERA", 75.f);
		const TickGroupDefinition POST_CAMERA			= TickGroupDefinition("POST_CAMERA", 80.f);
		const TickGroupDefinition AUDIO					= TickGroupDefinition("AUDIO", 90.f);

		static const TickGroups& get();
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tick groups are meant to be a static and near-compile-time concept. But I wanted 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TickGroupManager final
	{
	public:
		/** Key allowing only the GameBase to call certain functions */
		struct GameBaseKey { private: GameBaseKey() {} friend class GameBase; };
		struct TickGroupKey { private: TickGroupKey() {} friend TickGroupDefinition; };
	public:
		void start_TickGroupRegistration(const GameBaseKey& privateKey);
		void stop_TickGroupRegistration(const GameBaseKey& privateKey);
		bool registerTickGroup(const TickGroupDefinition& tickgroup, const TickGroupKey& privateKey);
		bool areTickGroupsInitialized() { return bTickGroupsInitialized; }
		const std::vector<TickGroupDefinition>& getSortedTickGroups() { return localGroupCopies; }
	private:
		/** TickGroups can only be registered during startup, their priorities should be considered compile time constants. But this is architectured available to allow subclassing*/
		std::vector<const TickGroupDefinition*> groupsToSort;
		std::vector<TickGroupDefinition> localGroupCopies;
		bool bTickgroupRegistrationActive = false;
		bool bTickGroupsInitialized = false;
	};

	


}
