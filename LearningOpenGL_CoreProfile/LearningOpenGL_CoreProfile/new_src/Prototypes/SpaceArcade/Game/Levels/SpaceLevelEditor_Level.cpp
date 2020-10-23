#include "SpaceLevelEditor_Level.h"
#include "../SpaceArcade.h"
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../GameSystems/SAUISystem_Editor.h"
#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../GameSystems/SAModSystem.h"
#include <string>
#include "LevelConfigs/SpaceLevelConfig.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../Tools/SAUtilities.h"
#include "../GameEntities/AvoidMesh.h"
#include "../Environment/Star.h"
#include "../Environment/Planet.h"
#include "../Environment/StarField.h"
#include "../Environment/Nebula.h"
#include "../../Rendering/Camera/Texture_2D.h"

namespace SA
{

	namespace
	{
		char tempTextBuffer[4096];
		const char* textWithIdx(const char* const text, size_t idx) 
		{
			snprintf(tempTextBuffer, sizeof(tempTextBuffer), text, idx);
			return tempTextBuffer;
		};
	}


	void SpaceLevelEditor_Level::startLevel_v()
	{
		Parent::startLevel_v();

		SpaceArcade& game = SpaceArcade::get();

		game.getEditorUISystem()->onUIFrameStarted.addStrongObj(sp_this(), &SpaceLevelEditor_Level::renderUI_editor);
		//game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePlayerCreated);
		//game.getWindowSystem().onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePrimaryWindowChanging);

		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &SpaceLevelEditor_Level::handlePlayerCreated);
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}
	}

	void SpaceLevelEditor_Level::endLevel_v()
	{
		Parent::endLevel_v();

		SpaceArcade& game = SpaceArcade::get();
		game.getEditorUISystem()->onUIFrameStarted.removeStrong(sp_this(), &SpaceLevelEditor_Level::renderUI_editor);

	}

	void SpaceLevelEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		Parent::render(dt_sec, view, projection);

		//for (const auto& asteroidDatum : asteroidData)
		//{
		//	if (asteroidDatum.demoAsteroid)
		//	{
		///		///don't do this, just use normal level rendering if possible
		//		//asteroidDatum.demoAsteroid->render(parentWorldShader);
		//	}
		//}
	}

	void SpaceLevelEditor_Level::renderUI_editor()
	{
		ImGui::SetNextWindowPos(ImVec2{ 25, 25 });
		//ImGui::SetNextWindowSize(ImVec2{ 400, 600 });
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoMove;
		//flags |= ImGuiWindowFlags_NoResize;
		//flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Level Editor!", nullptr, flags);
		{
			ImGui::Text("Press T to toggle between mouse and camera.");
			ImGui::Text("Press G to toggle environment debugging.");

			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();

			std::string activeModText = activeMod ? activeMod->getModName() : "None";
			ImGui::Text("Active Mod: "); ImGui::SameLine(); ImGui::Text(activeModText.c_str());

			std::string activeConfigText = activeLevelConfig ? activeLevelConfig->getName() : "None";
			ImGui::Text("Loaded Level Config: "); ImGui::SameLine(); ImGui::Text(activeConfigText.c_str());

			if (activeLevelConfig)
			{
				ImGui::SameLine();
				if (ImGui::Button("Save Level"))
				{
					saveActiveConfig();
				}
			}

			ImGui::Separator();

			/////////////////////////////////////
			// Collapsible headers
			/////////////////////////////////////
			if (ImGui::CollapsingHeader("LEVEL LOADING/SAVING", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_levelLoadingSaving();
			}
			if (ImGui::CollapsingHeader("WORLD OBJECTS", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_avoidMeshPlacement();
			}
			if (ImGui::CollapsingHeader("environment ", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_environment();
			}
		}
		ImGui::End();

	}

	void SpaceLevelEditor_Level::renderUI_levelLoadingSaving()
	{
		/////////////////////////////////////////////////
		// Spawn Configs List
		/////////////////////////////////////////////////
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		static int selectedLevelIndex = -1;
		ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, { 0.1f, 0.1f, 0.1f, 1.f });
		ImGui::BeginChild("SavedConfirgurationsMenu", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.75f, 100), false, file_load_wndflags);
		{
			if (activeMod)
			{
				const std::map<std::string, sp<SpaceLevelConfig>>& levelConfigs = activeMod->getLevelConfigs();
				if (levelConfigs.size() > 0)
				{
					int curConfigIdx = 0;
					for (const auto& kvPair : levelConfigs)
					{
						if (ImGui::Selectable(kvPair.first.c_str(), curConfigIdx == selectedLevelIndex))
						{
							activeLevelConfig = kvPair.second;
							selectedLevelIndex = curConfigIdx;
							onActiveLevelConfigSet(activeLevelConfig);
						}
						++curConfigIdx;
					}
				}
				else
				{
					ImGui::Text("No Level configs in this mod.");
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
		ImGui::BeginChild("level interaction buttons", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.25f, 100), false, 0);
		{
			/////////////////////////////////////////////////
			// Checkbox auto-safe
			/////////////////////////////////////////////////
			//ImGui::Checkbox("Autosave", &bAutoSave);

			/////////////////////////////////////////////////
			// Button Save
			/////////////////////////////////////////////////
			//if (ImGui::Button("Save"))
			//{
			//	TODO_do_setup_for_save_like_other_save_button____eg_replace_avoidance_meshes;
			//	activeConfig->save();
			//}
			///////////////////////////////////////////////////
			//// Button Delete
			///////////////////////////////////////////////////
			//if (activeConfig)
			//{
			//	if (activeConfig->isDeletable())
			//	{
			//		if (ImGui::Button("Delete"))
			//		{
			//			ImGui::OpenPopup("DeleteLevelConfigPopup");
			//		}
			//	}
			//	else
			//	{
			//		ImGui::Text("Cannot Delete");
			//	}

			//}

			if (ImGui::BeginPopupModal("DeleteLevelConfigPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (activeLevelConfig)
				{
					ImGui::Text("DELETE:"); ImGui::SameLine(); ImGui::Text(activeLevelConfig->getName().c_str());
					ImGui::Text("WARNING: this operation is irreversible!");
					ImGui::Text("Do you really want to delete this level?");

					if (ImGui::Button("DELETE", ImVec2(120, 0)))
					{
						if (activeMod)
						{
							activeMod->deleteLevelConfig(activeLevelConfig);
							activeLevelConfig = nullptr;
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

		static char configNameBuffer_file[512 + 1];
		ImGui::InputText("New Level File Name", configNameBuffer_file, 512, ImGuiInputTextFlags_CharsNoBlank);

		static char configNameBuffer_user[512 + 1];
		ImGui::InputText("Level User Facing Name", configNameBuffer_user, 512, ImGuiInputTextFlags_CharsNoBlank);
		
		if (ImGui::Button("Add New Level"))
		{
			std::string fileNameCopy = configNameBuffer_file;
			std::string userNameCopy = configNameBuffer_user;

			configNameBuffer_user[0] = 0; //clear buffer, imgui will move trailing null terminating zero
			configNameBuffer_file[0] = 0;

			createNewLevel(fileNameCopy, userNameCopy);
		}

		ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
		ImGui::Separator();

	}

	void SpaceLevelEditor_Level::renderUI_avoidMeshPlacement()
	{
		/////////////////////////////////////////////////
		// asteroid placements List
		/////////////////////////////////////////////////
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		//ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;
		//ImGui::BeginChild("Asteroids", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.75f, 100), false, file_load_wndflags);
		{
			ImGui::Text("Placed Avoid Meshes:");
			int iter_AvoidMeshIdx = 0;
			for (AvoidMeshEditorData& ad : avoidMeshData)
			{
				if (ImGui::Selectable(textWithIdx("avoidMesh %d", iter_AvoidMeshIdx), iter_AvoidMeshIdx == selectedAvoidMesh))
				{
					selectedAvoidMesh = iter_AvoidMeshIdx;
				}
				++iter_AvoidMeshIdx;
			}

			if (ImGui::Button("Add Avoid Mesh"))
			{
				avoidMeshData.emplace_back();
				AvoidMeshEditorData& newData = avoidMeshData.back();
			}
			////////////////////////////////////////////////////////
			//selected asteroid configuration menu
			////////////////////////////////////////////////////////
			if (Utils::isValidIndex(avoidMeshData, selectedAvoidMesh))
			{
				ImGui::Text("Current Asteroid");
				AvoidMeshEditorData& ad = avoidMeshData[selectedAvoidMesh];

				if (activeMod)
				{
					ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;
					ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, { 0.1f, 0.1f, 0.1f, 1.f });
					ImGui::BeginChild("Asteroid Model", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.75f, 100), false, file_load_wndflags);
					const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
					if (spawnConfigs.size() > 0)
					{
						int curConfigIdx = 0;
						for (const auto& kvPair : spawnConfigs)
						{
							const std::string& iterConfigName = kvPair.first;
							if (ImGui::Selectable(iterConfigName.c_str(), iterConfigName == ad.spawnConfigName))
							{
								//found active config or set new active config
								if (bool bJustUpdated = (ad.spawnConfigName != iterConfigName)) //becareful changing this, as we're making objects here
								{
									ad.spawnConfigName = iterConfigName;
									Transform newXform;
									if (ad.demoMesh)
									{
										//destroy old asteroid
										newXform = ad.demoMesh->getTransform(); //use old asteroids transform when changing config
										ad.demoMesh->destroy(); //get rid of old asteroid object, we're going to make a new won if valid 
										//unspawnEntity(ad.demoAsteroid); //LEVEL needs to be able to do this on its own, not just for ships
									}

									//make an asteroid since we did a selection
									if (kvPair.second)
									{
										//AvoidMesh::SpawnData asteroidSpawnParams;
										//asteroidSpawnParams.spawnTransform = newXform;
										//asteroidSpawnParams.spawnConfig = kvPair.second;
										//ad.demoAsteroid = spawnEntity<AvoidMesh>(asteroidSpawnParams);
										ad.demoMesh = spawnEditorDemoAvoidanceMesh(ad.spawnConfigName, newXform);
									}
								}
							}
							++curConfigIdx;
						}
					}
					else
					{
						ImGui::Text("No spawn configs in this mod.");
					}
					ImGui::EndChild();
					ImGui::PopStyleColor();
				}
				else
				{
					ImGui::Text("No Active Mod Loaded.");
				}

				if (ad.demoMesh)
				{
					Transform newXform = ad.demoMesh->getTransform(); 
					bool bDirty = false;

					if (ImGui::InputFloat3("position", &newXform.position.x)){bDirty = true;}
					if (ImGui::InputFloat3("scale", &newXform.scale.x)) { bDirty = true; }

					static glm::vec3 rotProxy_eulerAngles = glm::vec3(0.f);
					rotProxy_eulerAngles.x = glm::degrees(glm::pitch(newXform.rotQuat));
					rotProxy_eulerAngles.y = glm::degrees(glm::yaw(newXform.rotQuat));
					rotProxy_eulerAngles.z = glm::degrees(glm::roll(newXform.rotQuat));
					if (ImGui::InputFloat3("rotation", &rotProxy_eulerAngles.x)) { bDirty = true; }

					if (bDirty)
					{
						newXform.rotQuat = glm::quat(radians(rotProxy_eulerAngles));
						ad.demoMesh->setTransform(newXform);
					}
				}
			}
		}
		//ImGui::EndChild();

		ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
		ImGui::Separator();

	}

	void SpaceLevelEditor_Level::renderUI_environment()
	{
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod && activeLevelConfig)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// stars
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			{
				ImGui::Dummy(ImVec2(0, 10));

				bool bDirty_stars = false;
				ImGui::Text("Local Stars");
				static size_t selectedStarIdx = 0;

				//select a star
				if (localStarData_editor.size() > 0)
				{
					size_t starIdx = 0;
					for (const StarData& star : localStarData_editor)
					{
						if (ImGui::Selectable(textWithIdx("star [%d]", starIdx), starIdx == selectedStarIdx))
						{
							if (selectedStarIdx != starIdx){selectedStarIdx = starIdx;}
						}
						++starIdx;
					}
				}
				if (ImGui::Button("Push star"))
				{
					localStarData_editor.emplace_back();
					bDirty_stars = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Pop star") && localStarData_editor.size() > 0)
				{
					localStarData_editor.pop_back();
					bDirty_stars = true;
				}

				//modify selected star
				if (Utils::isValidIndex(localStarData_editor, selectedStarIdx))
				{
					StarData& selectedStar = localStarData_editor[selectedStarIdx];
				

					selectedStar.color = selectedStar.color.has_value() ? selectedStar.color : glm::vec3(1.f); //make sure optional has value
					bDirty_stars |= ImGui::ColorPicker3("star color", &selectedStar.color->r);

					selectedStar.offsetDir = selectedStar.offsetDir.has_value() ? selectedStar.offsetDir : glm::vec3(1.f); //make sure optional has value
					bDirty_stars |= ImGui::InputFloat3("star offsetDir", &selectedStar.offsetDir->x);
				
					selectedStar.offsetDistance = selectedStar.offsetDistance.has_value() ? selectedStar.offsetDistance : 1.f; //make sure optional has value
					bDirty_stars |= ImGui::InputFloat("star offset distance", &selectedStar.offsetDistance.value());

					if (bDirty_stars)
					{
						updateLevelData_Star();
					}
				}
			}
			ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
			ImGui::Separator();

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// planets
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			{
				ImGui::Dummy(ImVec2(0, 10));

				bool bDirty_Planet = false;
				static size_t selectedPlanetIdx = 0;

				//select a planet
				if (planets_editor.size() > 0)
				{
					size_t planetIdx = 0;
					for (const PlanetData& planet : planets_editor)
					{
						if (ImGui::Selectable(textWithIdx("planet [%d]", planetIdx), planetIdx == selectedPlanetIdx))
						{
							if (selectedPlanetIdx != planetIdx) { selectedPlanetIdx = planetIdx; }
						}
						++planetIdx;
					}
				}
				if (ImGui::Button("Push planet"))
				{
					planets_editor.emplace_back();
					bDirty_Planet = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Pop planet") && planets_editor.size() > 0)
				{
					planets_editor.pop_back();
					bDirty_Planet = true;
				}

				//modify selected planet
				if (Utils::isValidIndex(planets_editor, selectedPlanetIdx))
				{
					PlanetData& selectedPlanet = planets_editor[selectedPlanetIdx];

					selectedPlanet.offsetDir = selectedPlanet.offsetDir.has_value() ? selectedPlanet.offsetDir : glm::vec3(1.f); //make sure optional has value
					bDirty_Planet |= ImGui::InputFloat3("planet offsetDir", &selectedPlanet.offsetDir->x);

					selectedPlanet.offsetDistance = selectedPlanet.offsetDistance.has_value() ? selectedPlanet.offsetDistance : 1.f; //make sure optional has value
					bDirty_Planet |= ImGui::InputFloat("planet offset distance", &selectedPlanet.offsetDistance.value());

					selectedPlanet.bHasCivilization = selectedPlanet.bHasCivilization.has_value() ? selectedPlanet.bHasCivilization : false; //make sure optional has value
					bDirty_Planet |= ImGui::Checkbox("bHasCivilization", &selectedPlanet.bHasCivilization.value());

					static bool bSpecifyPlanetTexture = false;
					ImGui::Checkbox("set planet texture", &bSpecifyPlanetTexture);

					if (bSpecifyPlanetTexture)
					{
						selectedPlanet.texturePath = selectedPlanet.texturePath.has_value() ? selectedPlanet.texturePath : "no mod relative path";
						static char texturePathBuffer[2046];
						bDirty_Planet |= ImGui::InputText("texture file path", texturePathBuffer, sizeof(texturePathBuffer));
					}
					else if (bool bNeedsClearing = selectedPlanet.texturePath.has_value())
					{
						selectedPlanet.texturePath = std::nullopt; //clear the texture
						bDirty_Planet = true;
					}

					if (bDirty_Planet)
					{
						updateLevelData_Planets();
					}
				}
			}
			ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
			ImGui::Separator();

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// nebula
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			{
				ImGui::Dummy(ImVec2(0, 10));
				bool bDirty_Nebula = false;
				bool bSelectedNebulaThisFrame = false;
				static size_t selectedNebulaIdx = 0;

				//select a nebula
				if (nebulaData_editor.size() > 0)
				{
					size_t nebulaIdx = 0;
					for (const NebulaData& nebula : nebulaData_editor)
					{
						if (ImGui::Selectable(textWithIdx("nebula [%d]", nebulaIdx), nebulaIdx == selectedNebulaIdx))
						{
							if (selectedNebulaIdx != nebulaIdx) 
							{ 
								selectedNebulaIdx = nebulaIdx; 
								bSelectedNebulaThisFrame = true;
							}
						}
						++nebulaIdx;
					}
				}
				if (ImGui::Button("Push nebula"))
				{
					nebulaData_editor.emplace_back();
					nebulaData_editor.back().transform.position = glm::vec3(1.f);
					bDirty_Nebula = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Pop nebula") && nebulaData_editor.size() > 0)
				{
					nebulaData_editor.pop_back();
					bDirty_Nebula = true;
				}

				//modify selected nebula
				if (Utils::isValidIndex(nebulaData_editor, selectedNebulaIdx))
				{
					NebulaData& selectedNebulaData = nebulaData_editor[selectedNebulaIdx];

					float sliderBounds = 10.f;

					bDirty_Nebula |= ImGui::ColorPicker3("nebula color", &selectedNebulaData.tintColor.r);
					bDirty_Nebula |= ImGui::SliderFloat3("nebula position", &selectedNebulaData.transform.position.x, -sliderBounds, sliderBounds);
					{ 
						//glm::vec3 nebDirection = selectedNebulaData.transform.position; //relative to origin
						//float nebDist = glm::length(nebDirection);
						//const float small_number = 0.0001f;
						//nebDirection = nebDist <= small_number ? glm::vec3(0.01f) : nebDirection;
						//nebDist = nebDist <= 0.0001 ? 0.1f : nebDist; //zero check
						//bDirty_Nebula |= ImGui::SliderFloat3("nebula direction", &nebDirection.x, -1.f, 1.f);
						//bDirty_Nebula |= ImGui::SliderFloat("nebula dist", &nebDist, -sliderBounds, sliderBounds);
					}
					bDirty_Nebula |= ImGui::SliderFloat3("nebula scale", &selectedNebulaData.transform.scale.x, 0, 2*	sliderBounds);

					//selectedNebula.offsetDir = selectedNebula.offsetDir.has_value() ? selectedNebula.offsetDir : glm::vec3(1.f); //make sure optional has value
					//bDirty_Nebula |= ImGui::InputFloat3("nebula offsetDir", &selectedNebula.offsetDir->x);

					//selectedNebula.offsetDistance = selectedNebula.offsetDistance.has_value() ? selectedNebula.offsetDistance : 1.f; //make sure optional has value
					//bDirty_Nebula |= ImGui::InputFloat("nebula offset distance", &selectedNebula.offsetDistance.value());

					static bool bUseDefaultNebulaTexture = true;
					ImGui::Checkbox("use default nebula texture", &bUseDefaultNebulaTexture);
					if (!bUseDefaultNebulaTexture)
					{
						static char texturePathBuffer[2046];
						if (bSelectedNebulaThisFrame)
						{
							//copy previous string / clear out field
							size_t savedStrSize = selectedNebulaData.texturePath.length() + 1;
							std::memcpy(texturePathBuffer, selectedNebulaData.texturePath.c_str(), sizeof(texturePathBuffer) <= savedStrSize ? sizeof(texturePathBuffer) : savedStrSize);
							texturePathBuffer[sizeof(texturePathBuffer) - 1] = 0; //make sure we always null terminate since we're using memcpy
						}
						if (ImGui::InputText("nebula texture file path", texturePathBuffer, sizeof(texturePathBuffer)))
						{
							selectedNebulaData.texturePath = texturePathBuffer;
							bDirty_Nebula = true;
						}
					}
					else 
					{
						std::string defaultTexturePath = /*activeMod->getModDirectoryPath() + */"Assets/Textures/nebula2.png";
						//std::string defaultTexturePath = "GameData/mods/SpaceArcade/Assets/Textures/nebula2.png";
						if (selectedNebulaData.texturePath != defaultTexturePath)
						{
							selectedNebulaData.texturePath = defaultTexturePath;
							bDirty_Nebula = true;
						}
					}

					if (Utils::isValidIndex(nebulae, selectedNebulaIdx))
					{
						if (sp<Nebula> selectedNebulaPtr = nebulae[selectedNebulaIdx])
						{
							const sp<Texture_2D>& texture = selectedNebulaPtr->getTexture();
							if (texture && texture->isLoadedSuccessfully())	{ImGui::Text("Nebula texture loaded");}
							else											{ImGui::Text("Nebula texture failed to load");}
						}
					}

					if (bDirty_Nebula)
					{
						//convert direction * distance to regulard transform.
						//if (Utils::isValidIndex(nebulaData_editor, selectedNebulaIdx))
						//{
						//	nebulaData_editor[selectedNebulaIdx].transform.position = nebDist * glm::normalize(nebDirection);
						//}
						
						updateLevelData_Nebula();
					}
				}
			}
			ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
			ImGui::Separator();

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// starfield
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//TODO this requires serialization of the color scheme for the star field, perhaps can just be 3 floats on the level config.
			//	generate approach is:
			//	 1. generate star field (generateStarField(), will require friend class SpaceLevelEditor_Level;)
			//	 2. deallocation of gpu resources
			//	 3. allocate gpu resources again
			//	 ... I think ...
		}
		else
		{
			ImGui::Text("No Active Level|Mod");
		}

		ImGui::Dummy(ImVec2(0, 20)); //bottom spacing
		ImGui::Separator();
	}

	void SpaceLevelEditor_Level::onActiveLevelConfigSet(const sp<SpaceLevelConfig>& newConfig)
	{
		//clear old data before we resize the array
		for (AvoidMeshEditorData& avoidMesh : avoidMeshData)
		{
			if (avoidMesh.demoMesh)
			{
				unspawnEntity(avoidMesh.demoMesh);
			}
		}

		avoidMeshData.clear();

		if (newConfig)
		{
			for (const WorldAvoidanceMeshData& configAvoidMeshDatum : newConfig->getAvoidanceMeshes())
			{
				avoidMeshData.emplace_back();
				AvoidMeshEditorData& newEditorMeshData = avoidMeshData.back();

				newEditorMeshData.spawnConfigName = configAvoidMeshDatum.spawnConfigName;
				newEditorMeshData.demoMesh = spawnEditorDemoAvoidanceMesh(configAvoidMeshDatum.spawnConfigName, configAvoidMeshDatum.spawnTransform);
			}

			localStarData_editor = newConfig->stars;
			planets_editor = newConfig->planets;
			nebulaData_editor = newConfig->nebulaData;

			//force refresh so we render correct things 
			updateLevelData_Planets();
			updateLevelData_Star();
			updateLevelData_Nebula();
			

		}
		else
		{
			localStarData_editor.clear();
			planets_editor.clear();
		}
	}

	sp<SA::AvoidMesh> SpaceLevelEditor_Level::spawnEditorDemoAvoidanceMesh(const std::string& spawnConfigName, const Transform& xform)
	{
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod)
		{
			const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
			if (spawnConfigs.size() > 0)
			{
				if(const auto& kvPair  = spawnConfigs.find(spawnConfigName); kvPair != spawnConfigs.end())
				{
					const std::string& iterConfigName = kvPair->first;

					AvoidMesh::SpawnData asteroidSpawnParams;
					asteroidSpawnParams.spawnTransform = xform;
					asteroidSpawnParams.spawnConfig = kvPair->second;
					return spawnEntity<AvoidMesh>(asteroidSpawnParams);
				}
			}
		}

		return nullptr;
	}

	void SpaceLevelEditor_Level::writeConfigAvoidMeshes()
	{
		if (activeLevelConfig)
		{
			activeLevelConfig->avoidanceMeshes.clear();

			for (const AvoidMeshEditorData& editorData : avoidMeshData)
			{
				WorldAvoidanceMeshData newData;
				newData.spawnConfigName = editorData.spawnConfigName;
				newData.spawnTransform = editorData.demoMesh ? editorData.demoMesh->getTransform() : Transform();
				activeLevelConfig->avoidanceMeshes.push_back(newData);
			}

			//activeConfig->numAvoidanceMeshs = avoidanceMeshes.size();
		}
	}

	void SpaceLevelEditor_Level::updateLevelData_Star()
	{
		if (activeLevelConfig)
		{
			if (localStars.size() > localStarData_editor.size())
			{
				//trim off extra stars, do not grow -- we will do that below to prevent null pointers
				localStars.resize(localStarData_editor.size());
			}

			for (size_t sunIdx = 0; sunIdx < localStarData_editor.size(); ++sunIdx)
			{
				while (!Utils::isValidIndex(localStars, sunIdx))
				{
					localStars.push_back(new_sp<Star>());
				}
			
				localStars[sunIdx]->ldr_color = *localStarData_editor[sunIdx].color;
				localStars[sunIdx]->updateXformForData(*localStarData_editor[sunIdx].offsetDir, *localStarData_editor[sunIdx].offsetDistance);

				//copy this to config, so if we save the data gets serialized to json
				//activeLevelConfig->stars = localStarData_editor;
				//activeLevelConfig->numStars = localStarData_editor.size();
			}

			refreshStarLightMapping();
		}
	}

	void SpaceLevelEditor_Level::updateLevelData_Planets()
	{
		if (activeLevelConfig)
		{
			if (planets.size() > planets_editor.size())
			{
				//trim off extra stars, do not grow -- we will do that below to prevent null pointers
				planets.resize(planets_editor.size());
			}

			for (size_t planetIdx = 0; planetIdx < planets_editor.size(); ++planetIdx)
			{
				Planet::Data initData;
				if(!Utils::isValidIndex(planets, planetIdx))
				{
					const sp<RNG>& generationRNG = getGenerationRNG();
					std::vector<sp<Planet>> generatedPlanets = makeRandomizedPlanetArray(*generationRNG); //unfortunately generates an arry, but that's okay will just take first. bit of a hack. :(
					planets.push_back(generatedPlanets[0]); //only take a single element from the generated array
				}
				copyPlanetDataToInitData(planets_editor[planetIdx], initData);

				planets[planetIdx]->setOverrideData(initData);
				planets[planetIdx]->updateTransformForData(*planets_editor[planetIdx].offsetDir, *planets_editor[planetIdx].offsetDistance);

				//don't do this naymore... doesn't make sense to read the data then write it back ... thae work flow is isn't good
				//copy this to config, so if we save the data gets serialized to json
				//activeLevelConfig->planets = planets_editor;
				//activeLevelConfig->numPlanets = planets_editor.size();
			}
		}
	}

	void SpaceLevelEditor_Level::updateLevelData_Nebula()
	{
		if (activeLevelConfig)
		{
			if (nebulae.size() > nebulaData_editor.size())
			{
				//trim off extra nebula, do not grow -- we will do that below to prevent null pointers
				nebulae.resize(nebulaData_editor.size());
			}

			size_t nebulaIdx = 0;
			for (const NebulaData& nebulaData : nebulaData_editor)
			{
				Nebula::Data overrideData;
				overrideData.textureRelativePath = nebulaData.texturePath;
				overrideData.tintColor = nebulaData.tintColor;

				if (!Utils::isValidIndex(nebulae, nebulaIdx))
				{
					nebulae.push_back(new_sp<Nebula>(overrideData));
				}

				const sp<Nebula>& nebula = nebulae[nebulaIdx];
				nebula->setXform(nebulaData.transform);
				nebula->setOverrideData(overrideData);

				++nebulaIdx;
			}

			//write data back
			//activeLevelConfig->nebulaData = nebulaData_editor;
		}
	}

	void SpaceLevelEditor_Level::saveActiveConfig()
	{
		//don't call updates here anymore... that is also reading the config
		writeConfigAvoidMeshes();

		//updateLevelData_Star();
		//updateLevelData_Planets();
		//updateLevelData_Nebula();

		//write star data
		activeLevelConfig->stars = localStarData_editor;
		activeLevelConfig->numStars = localStarData_editor.size();

		//write planet data
		activeLevelConfig->planets = planets_editor;
		activeLevelConfig->numPlanets = planets_editor.size();

		//write nebula data
		activeLevelConfig->nebulaData = nebulaData_editor;

		activeLevelConfig->save();
	}

	void SpaceLevelEditor_Level::createNewLevel(const std::string& fileName, const std::string& userFacingName)
	{
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod && fileName.length() > 0)
		{
			sp<SpaceLevelConfig> newConfig = new_sp<SpaceLevelConfig>();
			newConfig->setNewFileName(fileName);
			newConfig->userFacingName = userFacingName.length() > 0 ? userFacingName : fileName;

			activeMod->addLevelConfig(newConfig);
		}
	}

	//void SpaceLevelEditor_Level::saveAvoidanceMesh(size_t index)
	//{
	//	if (activeConfig && Utils::isValidIndex(avoidMeshData,index))
	//	{
	//		const AvoidMeshEditorData& editorData = avoidMeshData[index];

	//		WorldAvoidanceMeshData newData;
	//		newData.spawnConfigName = editorData.spawnConfigName;
	//		newData.spawnTransform = editorData.demoMesh ? editorData.demoMesh->getTransform() : Transform{};
	//		
	//		activeConfig->replaceAvoidMeshData(index, newData);
	//	}
	//}

	void SpaceLevelEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &SpaceLevelEditor_Level::handleKey);
		//player->getInput().onButton.addWeakObj(sp_this(), &SpaceLevelEditor_Level::handleMouseButton);

		if (const sp<CameraBase>& camera = player->getCamera())
		{
			//configure camera to look at model
			camera->setPosition({ 5, 0, 0 });
			camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
			camera->setFar(1000.f);
			//cachedPlayerCamera = camera;
		}
	}

	void SpaceLevelEditor_Level::handleKey(int key, int state, int modifier_keys, int scancode)
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

			if(key == GLFW_KEY_G)
			{
				//make the environment on a similar scale as the ships for debugging.
				bool bNavigable = !starField->getForceCentered();
				starField->setForceCentered(bNavigable);

				for (const sp<Star>& star : localStars)
				{
					star->setForceCentered(bNavigable);
				}

				for (const sp<Planet>& planet : planets)
				{
					planet->setForceCentered(bNavigable); //#TODO probably a good idea at this point to create an "CelestialEnvironmentObj" base class for the shared functionality
				}

				for (const sp<Nebula>& nebulum : nebulae)
				{
					nebulum->setForceCentered(bNavigable);
				}
			}
		}
	}

}

