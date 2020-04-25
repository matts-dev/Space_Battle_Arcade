#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"

#include<cstdio>

#include "DeveloperConsole.h"
#include "../CheatSystemBase.h"
#include "../Input/SAInput.h"
#include "../SAWindowSystem.h"
#include "../SAGameBase.h"
#include "../SAPlayerSystem.h"
#include "../SAPlayerBase.h"
#include "../SAGameEntity.h"
#include "../../Rendering/SAWindow.h"
#include "../../Game/SpaceArcade.h"
#include "../../Game/SAUISystem.h"
#include "../../Rendering/Camera/SACameraBase.h"

namespace SA
{

	void DeveloperConsole::toggle()
	{
		//not sure if we should do "toggles" as they end up being really confusing when there are multiple users involved
		bConsoleOpen = !bConsoleOpen;

		suggestedSelectionIdx = -1;

		if (bConsoleOpen)
		{
			bForceFocusNextFrame = true;
			renderFrameIdx = 0;

			cacheCommands();

			if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(owningPlayerIdx))
			{
				player->getInput().suspendInput(true);
				if (const sp<CameraBase>& camera = player->getCamera())
				{
					cachedWasInCursorMode = camera->isInCursorMode();
					camera->setCursorMode(true);
				}
			}
		}
		else
		{
			if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(owningPlayerIdx))
			{
				player->getInput().suspendInput(false);

				if (const sp<CameraBase>& camera = player->getCamera())
				{
					camera->setCursorMode(cachedWasInCursorMode);
				}
			}
		}

	}

	void DeveloperConsole::postConstruct()
	{
		//UI system needs to become an engine level system
		//this should also be renamed to "editor" ui system to differentiate it from game UI
		SpaceArcade& engine = SpaceArcade::get();
		const sp<UISystem_Editor>& uiSystem = engine.getUISystem();
		uiSystem->onUIFrameStarted.addWeakObj(sp_this(), &DeveloperConsole::handleUIFrameStarted);

		std::memset(textBuffer, 0, sizeof(textBuffer));
	}

	static int inputTextCallback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventKey == ImGuiKey_Tab)
		{
			if (DeveloperConsole* instance = reinterpret_cast<DeveloperConsole*>(data->UserData))
			{
				//sucks to do a string creation here, but doesn't suck too bad.
				std::string completionString = instance->handleTabPressedDuringInput();

				if (completionString.size() > 0)
				{
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, completionString.c_str());
				}
				return 1;
			}
		}
		return 0;
	}

	void DeveloperConsole::handleUIFrameStarted()
	{
		if (!bConsoleOpen)
		{
			return;
		}

		//ui system is not an engine level feature. So this function is public for now.

		static WindowSystem& windowSys = GameBase::get().getWindowSystem();

		if (const sp<Window>& primaryWindow = windowSys.getPrimaryWindow())
		{
			renderFrameIdx++;

			std::pair<int, int> fbSize = primaryWindow->getFramebufferSize();

			//render UI now that we have all the data we need
			{
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Render console terminal input text box
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				const float uiHeight = 35;
				const float uiWidth = 1000;
				ImVec2 terminalPosition = ImVec2{ 0, fbSize.second - uiHeight };
				ImGui::SetNextWindowSize(ImVec2{ uiWidth, uiHeight });
				ImGui::SetNextWindowPos(terminalPosition);
				ImGuiWindowFlags flags = 0;
				flags |= ImGuiWindowFlags_NoMove;
				flags |= ImGuiWindowFlags_NoResize;
				flags |= ImGuiWindowFlags_NoTitleBar;
				ImGui::Begin("console", nullptr, flags);
				{
					ImGui::PushItemWidth(ImGui::GetWindowWidth());

					//setting keyboard focus every frame will prevent the selectlabels from working correctly.
					if (bForceFocusNextFrame || nextFocusFrame == renderFrameIdx)
					{
						bForceFocusNextFrame = false;
						ImGui::SetKeyboardFocusHere(); //when console is up, always steal focus (prevents clicking during ship control from making it where player can't type)
					}

					if (ImGui::InputText("##hidelabel", textBuffer, sizeof(textBuffer), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion, &inputTextCallback, (void*)(this)))
					{
						//with this flags -- only enter will return true -- not each character
						std::string command = textBuffer;
						GameBase::get().getCheatSystem().parseCheat(command);
						std::memset(textBuffer, 0, sizeof(textBuffer)); //clear the current cheat
						updateHistory(command);
						//focusTextInput();
						toggle();
					}
				}
				ImGui::End();

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Render console terminal suggestions
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				bool bRenderSuggestions = true;
					
				if (bRenderSuggestions)
				{
					const float approximatedLineHeight = uiHeight;
					const float numSuggestionsToShow = float(MAX_SUGGESTION_LENGTH);
					const float suggestionHeight = approximatedLineHeight * numSuggestionsToShow;

					ImGui::SetNextWindowSize(ImVec2{ uiWidth, suggestionHeight });
					ImGui::SetNextWindowPos(ImVec2{ 0, terminalPosition.y - suggestionHeight });
					ImGuiWindowFlags flags = 0;
					flags |= ImGuiWindowFlags_NoMove;
					flags |= ImGuiWindowFlags_NoResize;
					flags |= ImGuiWindowFlags_NoTitleBar;
					ImGui::Begin("console suggestions", nullptr, flags);
					{
						ImGui::PushItemWidth(ImGui::GetWindowWidth());

						//TODO loop over cached commands and history to populate this list
						for (size_t iCmd = 0; iCmd < cachedCommands.size(); ++iCmd)
						{
							//#TODO #optimize filtering in draw loop is expensive, this should probably be done in a tick and only when buffer changes for perf on large lists

							//reverse find finds from index and lower; giving 0 means it only checks start position and no other stirng locations. 
							bool bPassesFilter = (cachedCommands[iCmd].rfind(textBuffer, 0) == 0) || !textBuffer[0];
							if (bPassesFilter)
							{
								//use helper buffer for null term safe during copy if it is selected
								snprintf(helperBuffer, sizeof(helperBuffer), "%s", cachedCommands[iCmd].c_str());
								if (ImGui::Selectable(helperBuffer, suggestedSelectionIdx == iCmd))
								{
									suggestedSelectionIdx = iCmd;
									
									std::memcpy(textBuffer, helperBuffer, glm::min(sizeof(textBuffer), sizeof(helperBuffer)));

									focusTextInput();
								}
							}
						}
					}
					ImGui::End();
				}
			}
		}
	}

	void DeveloperConsole::updateHistory(const std::string& command)
	{
		auto previousHistory = std::find(history.begin(), history.end(), command);
		if (previousHistory != history.end())
		{
			history.erase(previousHistory);
		}

		history.push_front(command);

		if (history.size() >= MAX_HISTORY_LENGTH)
		{
			history.pop_back();
		}

		cacheCommands();
	}

	void DeveloperConsole::focusTextInput()
	{
		//defer focus some number of frames so that selectables can do their logic
		nextFocusFrame = renderFrameIdx + focusFrameCycleNumber;
	}

	void DeveloperConsole::pollInput()
	{
		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			GLFWwindow* rawWindow = primaryWindow->get();

			static InputTracker inputTracker;
			inputTracker.updateState(rawWindow);

			if (inputTracker.isKeyJustPressed(rawWindow, GLFW_KEY_UP))
			{
				suggestedSelectionIdx = suggestedSelectionIdx == -1 ? cachedCommands.size() : suggestedSelectionIdx; //have up arrow start at bottom of list 
				suggestedSelectionIdx = glm::max<int32_t>(suggestedSelectionIdx - 1, 0);
			}
			if (inputTracker.isKeyJustPressed(rawWindow, GLFW_KEY_DOWN))
			{
				suggestedSelectionIdx = glm::min<int32_t>(suggestedSelectionIdx + 1, int32_t(cachedCommands.size()));
			}

			if (inputTracker.isMouseButtonJustPressed(rawWindow, GLFW_MOUSE_BUTTON_LEFT))
			{
				bFocusOnMouseRelease = true;
			}

			if (bFocusOnMouseRelease)
			{
				if(!inputTracker.isMouseButtonDown(rawWindow, GLFW_MOUSE_BUTTON_LEFT))
				{
					bFocusOnMouseRelease = false;
					focusTextInput();
				}
			}
		}
	}

	void DeveloperConsole::cacheCommands()
	{
		size_t reserveAmount = 0;

		CheatSystemBase& cheatManager = GameBase::get().getCheatSystem();
		reserveAmount += cheatManager.getNumCheats();
		reserveAmount += history.size();

		cachedCommands.reserve(reserveAmount);
		cachedCommands.clear();

		cheatManager.getAllCheats(cachedCommands);
		for (std::string& histItem : history)
		{
			cachedCommands.push_back(histItem);
		}
	}

	void DeveloperConsole::tick(float dt_sec)
	{
		if (bConsoleOpen)
		{
			//TODO filter text?


			pollInput();
		}
	}

	std::string DeveloperConsole::handleTabPressedDuringInput()
	{
		if (suggestedSelectionIdx >= 0 && suggestedSelectionIdx < int32_t(cachedCommands.size()))
		{
			const std::string& cmdStr = cachedCommands[size_t(suggestedSelectionIdx)];
			return cmdStr;
		}
		return std::string("");
	}

}

