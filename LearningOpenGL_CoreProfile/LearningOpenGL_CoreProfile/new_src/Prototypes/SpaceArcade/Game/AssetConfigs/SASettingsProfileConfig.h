#pragma once
#include "SAConfigBase.h"

namespace SA
{
	/** Performance scalability settings*/
	struct ScalabilitySettings
	{
		//defaults are tweaked based on my testing
		float multiplier_spawnComponentCooldownSec = 0.1f; 
		float multiplier_maxSpawnableShips = 1.0f;
	};

	class SettingsProfileConfig : public ConfigBase
	{
	protected:
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;
		virtual std::string getRepresentativeFilePath() override;
		std::string getIndexedName() const;
		//virtual const std::string& getName() const override;
	public:
		void setProfileIndex(size_t newIndex);
		void requestSave();
	public:
		bool bEnableDevConsole = true;
		float masterVolume = 1.f; //[0,1]
		size_t selectedTeamIdx = 0; //let player choose which team they want to play.
		float volumeMultiplier = 1.f;

		//performance tweaks
		ScalabilitySettings scalabilitySettings;
	private:
		size_t profileIndex = 0;
	};

}
