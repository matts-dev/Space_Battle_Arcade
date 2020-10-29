#pragma once
#include "SAConfigBase.h"

namespace SA
{
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
	private:
		size_t profileIndex = 0;
	};

}
