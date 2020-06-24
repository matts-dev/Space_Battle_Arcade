#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/RemoveSpecialMemberFunctionUtils.h"

namespace SA
{
	class SpaceLevelBase;
	struct EndGameParameters;

	class ServerGameMode_Base : public GameEntity
	{
	public:
		struct InitKey : public RemoveMoves {friend SpaceLevelBase; private: InitKey() {}};
	public:
		void initialize(const InitKey& key);
		void						setOwningLevel(const sp<SpaceLevelBase>& level);
		const wp<SpaceLevelBase>&	getOwningLevel() const { return owningLevel; }
		size_t getNumTeams() const { return numTeams; }
	protected:
		virtual void onInitialize(const sp<SpaceLevelBase>& level);
		void endGame(const EndGameParameters& endParameters);
	protected:
		size_t numTeams = 2;
	private:
		bool bBaseInitialized = false; //perhaps make static and reset since this will ever be created from single thread
		wp<SpaceLevelBase> owningLevel;
	};
}
