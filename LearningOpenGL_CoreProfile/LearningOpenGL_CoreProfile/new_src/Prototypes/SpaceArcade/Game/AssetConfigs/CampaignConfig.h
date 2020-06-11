#pragma once
#include "SAConfigBase.h"

namespace SA
{
	class CampaignConfig : public ConfigBase
	{
		using Parent = ConfigBase;
	protected:
		virtual void postConstruct() override;
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;
		virtual std::string getRepresentativeFilePath() override;
		std::string getIndexedName() const;
	public:
		/**Adding support for multiple campaigns in a mod, even though this may not be exposed to UI*/
		void setCampaignIndex(size_t newIndex);
		void requestSave();
	public://serialized fields
		//WARNING: refactoring these names will influence the out put json!
		std::string userFacingName = "Campaign";
		struct LevelData
		{
			std::string name;
			size_t tier = 0; //the horizontal level this should appear as (levels with same tier will be stacked ontop of each other)
			std::vector<size_t> outGoingPathIndices;
		};
		std::vector<LevelData> levels;
	private:
		size_t campaignIndex = 0;
	};
}