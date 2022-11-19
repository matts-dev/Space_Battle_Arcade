#pragma once
#include "GameFramework/SAGameEntity.h"
#include <cstdint>
#include <list>
#include <vector>

namespace SA
{
	class DeveloperConsole : public GameEntity
	{
		using Parent = GameEntity;
	public:
		void toggle();
		void tick(float dt_sec);
		bool isConsoleOpen() { return bConsoleOpen; }
		std::string handleTabPressedDuringInput();
	protected:
		virtual void postConstruct() override;
		void handleUIFrameStarted();
	private:
		void updateHistory(const std::string& command);
		void focusTextInput();
		void pollInput();
		void cacheCommands();
	private: //implementation details
		//setting focus to text input every frame prevents selectables in suggestions from working
		int renderFrameIdx = 0;
		int nextFocusFrame = 0;
		const int focusFrameCycleNumber = 3;
		bool bForceFocusNextFrame = false;
		bool bFocusOnMouseRelease = false;
		bool cachedWasInCursorMode = false;
	private:
		const size_t MAX_HISTORY_LENGTH = 10;
		const size_t MAX_SUGGESTION_LENGTH = 5;

		uint32_t owningPlayerIdx = 0;
		bool bConsoleOpen = false;
		char textBuffer[4096];
		char helperBuffer[4096];
		int32_t suggestedSelectionIdx = -1;
		std::list<std::string> history;
		std::vector<std::string> cachedCommands;
	};
}
