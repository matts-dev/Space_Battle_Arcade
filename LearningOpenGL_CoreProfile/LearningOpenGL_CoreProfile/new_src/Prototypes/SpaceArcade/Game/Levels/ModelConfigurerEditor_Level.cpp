#include "ModelConfigurerEditor_Level.h"
#include "../SpaceArcade.h"
#include "../SAUISystem.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../SAModSystem.h"
#include "../SASpawnConfig.h"
#include <filesystem>
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../Tools/DataStructures/SATransform.h"

namespace
{
	enum class LoadModelErrorState : uint8_t
	{
		NONE,
		ERROR_CHECKING_FILE_EXIST,
		ERROR_FILE_DOESNT_EXIST,
		ERROR_LOADING_MODEL,
	};
}

namespace SA
{
	void ModelConfigurerEditor_Level::startLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getUISystem()->onUIFrameStarted.addStrongObj(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);

		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePlayerCreated);
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}

		game.getModSystem()->onActiveModChanging.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleModChanging);

		model3DShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_AmbientLight, false);
	}

	void ModelConfigurerEditor_Level::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getUISystem()->onUIFrameStarted.removeStrong(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);

		model3DShader = nullptr;
	}

	void ModelConfigurerEditor_Level::handleUIFrameStarted()
	{
		ImGui::SetNextWindowPos(ImVec2{ 25, 25 });
		//ImGui::SetNextWindowSize(ImVec2{ 400, 600 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		//flags |= ImGuiWindowFlags_NoResize;
		//flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Model Editor!", nullptr, flags);
		{
			ImGui::Text("Press T to toggle between mouse and camera.");

			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();

			std::string activeModText = activeMod ? activeMod->getModName() : "None";
			ImGui::Text("Active Mod: "); ImGui::SameLine(); ImGui::Text(activeModText.c_str());

			std::string activeConfigText = activeSpawnConfig ? activeSpawnConfig->getName() : "None";
			ImGui::Text("Loaded Config: "); ImGui::SameLine(); ImGui::Text(activeConfigText.c_str());
			ImGui::Separator();

			/////////////////////////////////////
			// Collapsible headers
			/////////////////////////////////////
			if (ImGui::CollapsingHeader("ENTITY LOADING/SAVING", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_LoadingSavingMenu();
			}
			if (ImGui::CollapsingHeader("COLLISION"))
			{
				renderUI_Collision();
			}
			if (ImGui::CollapsingHeader("FIRING PROJECTILES"))
			{
				renderUI_Projectiles();
			}
			if (ImGui::CollapsingHeader("COLOR AND MATERIAL"))
			{
				renderUI_Material();
			}
			if (ImGui::CollapsingHeader("TEAM"))
			{
				renderUI_Team();
			}
		}
		ImGui::End();
	}

	void ModelConfigurerEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleKey);
		player->getInput().onButton.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleMouseButton);
		if (const sp<CameraBase>& camera = player->getCamera())
		{
			//configure camera to look at model
			//camera->setPosition({ 5, 0, 0 });
			//camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
		}
	}

	void ModelConfigurerEditor_Level::handleKey(int key, int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			if (key == GLFW_KEY_T)
			{
				const sp<PlayerBase>& zeroPlayer = SpaceArcade::get().getPlayerSystem().getPlayer(0);
				if (const sp<CameraBase>& camera = zeroPlayer->getCamera())
				{
					camera->setCursorMode(!camera->isInCursorMode());
				}
			}
		}
	}

	void ModelConfigurerEditor_Level::handleMouseButton(int button, int state, int modifier_keys)
	{

	}

	void ModelConfigurerEditor_Level::handleModChanging(const sp<Mod>& previous, const sp<Mod>& active)
	{
		renderModel = nullptr;
		activeSpawnConfig = nullptr;
	}

	void ModelConfigurerEditor_Level::renderUI_LoadingSavingMenu()
	{
		/////////////////////////////////////////////////
		// Spawn Configs List
		/////////////////////////////////////////////////
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		static int selectedSpawnConfigIdx = -1;
		ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, { 0.1f, 0.1f, 0.1f, 1.f });
		ImGui::BeginChild("SavedConfirgurationsMenu", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.75f, 100), false, file_load_wndflags);
		{
			if (activeMod)
			{
				const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
				if (spawnConfigs.size() > 0)
				{
					int curConfigIdx = 0;
					for (const auto& kvPair : spawnConfigs)
					{
						if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedSpawnConfigIdx))
						{
							activeSpawnConfig = kvPair.second;
							selectedSpawnConfigIdx = curConfigIdx;
							std::string modelPath = activeSpawnConfig->getModelFilePath();

							//may fail to load model if user provided bad path, either way we need to set to null or valid model
							renderModel = SpaceArcade::get().getAssetSystem().loadModel(modelPath.c_str());
						}
						++curConfigIdx;
					}
				}
				else
				{
					ImGui::Text("No spawn configs in this mod.");
				}
			}
			else
			{
				ImGui::Text("No Active Mod Loaded.");
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::BeginChild("model interaction buttons", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.25f, 100), false, 0);
		{
			/////////////////////////////////////////////////
			// Checkbox auto-safe
			/////////////////////////////////////////////////
			//ImGui::Checkbox("Autosave", &bAutoSave);

			/////////////////////////////////////////////////
			// Button Save
			/////////////////////////////////////////////////
			if (ImGui::Button("Save"))
			{
				activeSpawnConfig->save();
			}
			/////////////////////////////////////////////////
			// Button Delete
			/////////////////////////////////////////////////
			if (activeSpawnConfig)
			{
				if (activeSpawnConfig->isDeletable())
				{
					if (ImGui::Button("Delete"))
					{
						ImGui::OpenPopup("DeleteSpawnConfigPopup");
					}
				}
				else
				{
					ImGui::Text("Cannot Delete");
				}

			}
			if (ImGui::BeginPopupModal("DeleteSpawnConfigPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (activeSpawnConfig)
				{
					ImGui::Text("DELETE:"); ImGui::SameLine(); ImGui::Text(activeSpawnConfig->getName().c_str());
					ImGui::Text("WARNING: this operation is irreversible!");
					ImGui::Text("Do you really want to delete this spawn config?");

					if (ImGui::Button("DELETE", ImVec2(120, 0)))
					{
						if (activeMod)
						{
							activeMod->deleteSpawnConfig(activeSpawnConfig);
							activeSpawnConfig = nullptr;
							renderModel = nullptr;
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
				}
				if (ImGui::Button("CANCEL", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::EndChild();

		ImGui::Dummy(ImVec2(0, 20)); //bottom spacing

		static char configNameBuffer[512 + 1];
		ImGui::InputText("New Entity Name", configNameBuffer, 512, ImGuiInputTextFlags_CharsNoBlank);

		static char filepathBuffer[512 + 1];
		ImGui::InputText("3D model Filepath", filepathBuffer, 512, 0); //allow spaces to be in the filepath

		ImGui::Text("File path should be relative to mod; for space arcade:");
		ImGui::Text("eg /Assets/Models3D/Fighter/SGFighter.obj");
		if (configNameBuffer[0] != 0 && filepathBuffer[0] != 0)
		{
			if (activeMod)
			{
				/////////////////////////////////////////
				// Add Entity Button
				////////////////////////////////////////
				static LoadModelErrorState modelError = LoadModelErrorState::NONE;
				static std::string modRelativeModelPath;
				static std::string fullModelPath;
				if (ImGui::Button("Add Entity"))
				{
					//only update from buffers if they've clicked, otherwise it is the equivalent of updating strings every tick
					modRelativeModelPath = filepathBuffer;
					fullModelPath = activeMod->getModDirectoryPath() + modRelativeModelPath;

					std::error_code exists_ec;
					if (std::filesystem::exists(fullModelPath, exists_ec))
					{
						modelError = LoadModelErrorState::NONE;
						if (sp<Model3D> model = SpaceArcade::get().getAssetSystem().loadModel(fullModelPath.c_str()))
						{
							createNewSpawnConfig(configNameBuffer, fullModelPath);

							//update selection index after creating a new config
							const std::map<std::string, sp<SpawnConfig>>& refreshedSpawnConfigs = activeMod->getSpawnConfigs();
							if (refreshedSpawnConfigs.size() > 0)
							{
								int curConfigIdx = 0;
								for (const auto& kvPair : refreshedSpawnConfigs)
								{
									if (kvPair.first == configNameBuffer)
									{
										selectedSpawnConfigIdx = curConfigIdx;
										break;
									}
									++curConfigIdx;
								}
							}
						}
						else
						{
							log("Spawn Config Editor", LogLevel::LOG_ERROR, "Failed to load model");
							ImGui::OpenPopup("FailedToLoadModelPopup");
						}
					}
					else if (exists_ec)
					{
						modelError = LoadModelErrorState::ERROR_CHECKING_FILE_EXIST;
						ImGui::OpenPopup("FailedToLoadModelPopup");
					}
					else
					{
						modelError = LoadModelErrorState::ERROR_FILE_DOESNT_EXIST;
						ImGui::OpenPopup("FailedToLoadModelPopup");
					}

				}
				/////////////////////////////////////////
				// Failed to load model popup
				////////////////////////////////////////
				if (ImGui::BeginPopupModal("FailedToLoadModelPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Filepath provided:"); ImGui::SameLine(); ImGui::Text(modRelativeModelPath.c_str());
					ImGui::Text("Calculated path:"); ImGui::SameLine(); ImGui::Text(fullModelPath.c_str());
					if (modelError == LoadModelErrorState::ERROR_LOADING_MODEL)
					{
						ImGui::Text("ERROR: Failed to load model3d");
					}
					else if (modelError == LoadModelErrorState::ERROR_CHECKING_FILE_EXIST)
					{
						ImGui::Text("ERROR: filesystem exists()");
					}
					else if (modelError == LoadModelErrorState::ERROR_FILE_DOESNT_EXIST)
					{
						ImGui::Text("ERROR: file doesn't exist");
					}
					else
					{
						ImGui::Text("ERROR: unknown -- this if a bug with error handling");
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("Ok", ImVec2(120, 0)))
					{
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
			}
			else
			{
				ImGui::Text("Load a mod to add a spawn config");
			}
		}
		else
		{
			ImGui::Text("Complete the name and filepath fields to add a new spawn config.");
		}
			
		ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Collision()
	{
		ImGui::Separator();
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Projectiles()
	{
		ImGui::Separator();
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Material()
	{
		ImGui::Separator();
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Team()
	{
		ImGui::Separator();
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Text("place-holder");
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::createNewSpawnConfig(const std::string& configName, const std::string& fullModelPath)
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			renderModel = SpaceArcade::get().getAssetSystem().loadModel(fullModelPath.c_str());

			sp<SpawnConfig> newConfig = new_sp<SpawnConfig>();
			newConfig->name = configName;
			newConfig->fullModelFilePath = fullModelPath;
			newConfig->owningModDir = activeMod->getModDirectoryPath();
			newConfig->save();

			activeMod->addSpawnConfig(newConfig);

			activeSpawnConfig = newConfig;
		}
	}

	void ModelConfigurerEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		if (renderModel)
		{
			const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
			if (zeroPlayer)
			{
				const sp<CameraBase>& camera = zeroPlayer->getCamera();

				model3DShader->use();
				model3DShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				model3DShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				model3DShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
				model3DShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0));
				model3DShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
				model3DShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0));

				model3DShader->setUniform3f("cameraPosition", camera->getPosition());
				model3DShader->setUniform1i("material.shininess", 32);

				glm::mat4 model = glm::mat4(1.f);
				model = glm::translate(model, glm::vec3(5.f, 0.f, -5.f)); 
				model3DShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				renderModel->draw(*model3DShader);
			}
		}
	}
}

