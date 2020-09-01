#include "ModelConfigurerEditor_Level.h"

#include <filesystem>

#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"

#include "../SpaceArcade.h"
#include "../GameSystems/SAUISystem_Editor.h"
#include "../GameSystems/SAModSystem.h"
#include "../AssetConfigs/SASpawnConfig.h"
#include "../SAPrimitiveShapeRenderer.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/Input/SAInput.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Rendering/OpenGLHelpers.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../../../../Algorithms/SeparatingAxisTheorem/SATRenderDebugUtils.h"
#include "../../../../Algorithms/SeparatingAxisTheorem/ModelLoader/SATModel.h"
#include "../../GameFramework/SACollisionUtils.h"
#include "../../Rendering/Camera/SACameraFPS.h"
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../Tools/SACollisionHelpers.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Rendering/RenderData.h"
#include "../../Rendering/SAShader.h"

#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"
#include "../../Tools/SAUtilities.h"
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../Tools/color_utils.h"
#include <string>


namespace
{
	enum class LoadModelErrorState : uint8_t
	{
		NONE,
		ERROR_CHECKING_FILE_EXIST,
		ERROR_FILE_DOESNT_EXIST,
		ERROR_LOADING_MODEL,
	};

	char tempTextBuffer[4096];

}

using TriangleProcessor = SAT::DynamicTriangleMeshShape::TriangleProcessor;

namespace SA
{
	static bool bEnableCollisionTick = true;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A debug camera used to test collision within the model editor level
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CollisionDebugCamera final : public QuaternionCamera
	{
	public:
		CollisionDebugCamera();
	public:
		void updateTrackedLevelCollision(const SpawnConfig& debugSpawnConfig);
	protected:
		virtual void tick(float dt_sec) override;
	private:
		sp<CollisionData> debugWorldCollisionInfo;
		sp<CollisionData> cameraCollisionData;
	};

	CollisionDebugCamera::CollisionDebugCamera()
	{
		cameraCollisionData = createUnitCubeCollisionData();
	}

	void CollisionDebugCamera::tick(float dt_sec)
	{
		QuaternionCamera::tick(dt_sec);

		if (cameraCollisionData && debugWorldCollisionInfo)
		{
			Transform cameraNewXform;
			cameraNewXform.position = getPosition();
			cameraNewXform.scale = glm::vec3(3.f); //scale up the collision by some amount so can detect it before clipping with the model.
			//cameraNewXform.rotQuat; //ignore rotation as it shouldn't really matter for debugging purposes, we're just floating a cube around
			cameraCollisionData->updateToNewWorldTransform(cameraNewXform.getModelMatrix());

			using ShapeData = CollisionData::ShapeData;
			bool bCollision = false;
			size_t numAttempts = 3;
			size_t attempt = 0;
			do
			{
				attempt++;
				bCollision = false;

				glm::vec4 largestMTV = glm::vec4(0.f);
				float largestMTV_len2 = 0.f;
				for (const ShapeData& cameraShape :	 cameraCollisionData->getShapeData())
				{
					for (const ShapeData& worldShape : debugWorldCollisionInfo->getShapeData())
					{
						assert(cameraShape.shape && worldShape.shape);
						glm::vec4 mtv;
						if (SAT::Shape::CollisionTest(*cameraShape.shape, *worldShape.shape, mtv))
						{
							float mtv_len2 = glm::length2(mtv);
							if (mtv_len2 > largestMTV_len2)
							{
								largestMTV = mtv;
								largestMTV_len2 = mtv_len2;
								bCollision = true;
							}
						}
					}
				}
				if (bCollision)
				{
					cameraNewXform.position += glm::vec3(largestMTV);
					setPosition(cameraNewXform.position);
					cameraCollisionData->updateToNewWorldTransform(cameraNewXform.getModelMatrix());
				}
			} while (bCollision && attempt < numAttempts);
		}
	}

	void CollisionDebugCamera::updateTrackedLevelCollision(const SpawnConfig& debugSpawnConfig)
	{
		//#TODO something is cause causing a memory leak and we will crash here. Something is growing memory in this level. this should delete though -- perhaps dangling strong delegate?
		debugWorldCollisionInfo = nullptr;

		if (bEnableCollisionTick)
		{
			debugWorldCollisionInfo = debugSpawnConfig.toCollisionInfo(); 
			debugWorldCollisionInfo->updateToNewWorldTransform(glm::mat4(1.f)); //perhaps should just be done in "toCollisionInfo()?"
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Model Configurer level
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void ModelConfigurerEditor_Level::startLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getEditorUISystem()->onUIFrameStarted.addStrongObj(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);
		game.getPlayerSystem().onPlayerCreated.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePlayerCreated);
		game.getWindowSystem().onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handlePrimaryWindowChanging);

		levelCamera = new_sp<CollisionDebugCamera>();
		if (levelCamera)
		{
			//configure camera to look at model
			levelCamera->setPosition({ 5, 0, 0 });
			levelCamera->lookAt_v((levelCamera->getPosition() + glm::vec3{ -1, 0, 0 }));
			levelCamera->setFar(1000.f);
			//levelCamera->registerToWindowCallbacks_v(game.getWindowSystem().getPrimaryWindow());
		}
		if (const sp<SA::PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
		{
			handlePlayerCreated(player, 0);
		}

		game.getModSystem()->onActiveModChanging.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleModChanging);

		model3DShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_AmbientLight, false);
		collisionShapeShader = new_sp<SA::Shader>(SAT::DebugShapeVertSrc, SAT::DebugShapeFragSrc, false);
		shapeRenderer = new_sp<PrimitiveShapeRenderer>();

		if (!polyShape)
		{
			polyShape = new_sp<SAT::PolygonCapsuleShape>();
			cubeShape = new_sp<SAT::CubeShape>();
		}
		capsuleRenderer = new_sp<SAT::CapsuleRenderer>();
		sharedAvoidanceRenderer = new_sp<AvoidanceSphere>(1.0f, glm::vec3(0.f));
	}

	void ModelConfigurerEditor_Level::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getEditorUISystem()->onUIFrameStarted.removeStrong(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);

		//cleaning up OpenGL resources immediately, these should get cleaned up with their dtors when this dtor is hit; so probably unecessary to do here
		model3DShader = nullptr;
		collisionShapeShader = nullptr;
		shapeRenderer = nullptr;
		capsuleRenderer = nullptr;

		bUseCollisionCamera = false;
		updateCameras(); //restore the player camera
	}

	void ModelConfigurerEditor_Level::tick_v(float dt_sec)
	{
		LevelBase::tick_v(dt_sec);

		if (activeConfig)
		{
			////////////////////////////////////////////////////////
			// manually upate placement transforms
			////////////////////////////////////////////////////////
			glm::mat4 rootModelMat = activeConfig->getModelXform().getModelMatrix();
			static const auto applyTransformToPlacements = [](
				const std::vector<sp<ShipPlacementEntity>>& placementContainer,
				const glm::mat4& rootModelXform) 
			{
				for (const sp<ShipPlacementEntity>& placement : placementContainer)
				{
					if (placement)
					{
						placement->setParentXform(rootModelXform);
					}
				}
			};
			applyTransformToPlacements(placement_turrets, rootModelMat);
			applyTransformToPlacements(placement_communications, rootModelMat);
			applyTransformToPlacements(placement_defenses, rootModelMat);


		}
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

			std::string activeConfigText = activeConfig ? activeConfig->getName() : "None";
			ImGui::Text("Loaded Config: "); ImGui::SameLine(); ImGui::Text(activeConfigText.c_str());

			if (activeConfig)
			{
				ImGui::SameLine(); 
				if (ImGui::Button("Save Model"))
				{
					activeConfig->save();
				}
			}

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
			if (ImGui::CollapsingHeader("TEAM"))
			{
				renderUI_Team();
			}
			if (ImGui::CollapsingHeader("OBJECTIVES"))
			{
				renderUI_Objectives();
			}
			if (ImGui::CollapsingHeader("SPAWNING"))
			{
				renderUI_Spawning();
			}
			if (ImGui::CollapsingHeader("SOUNDS"))
			{
				renderUI_Sounds();
			}
			if (ImGui::CollapsingHeader("VIEWPORT UI", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_ViewportUI();
			}
		}
		ImGui::End();

		//-- #TODO I've identified by early outing that below(update tracked level collision) is causing a memory leak --

		//pretty terrible, but doing this every tick after UI has had change to update. This isn't so bad because this is just a model editor.
		//but it will have performance issues when the model grows more complex and has many collision shapes. But that would be bad for 
		//game performance, so this can be used as a proxy to know if the user is creating a model that will be bad for performance.
		//-- the alternative to doing this every tick is to have each ui element above that potentially changes collision cause a rebuild.
		//that's a lot of work for little gain at this point (it also muddies up the UI code as all widgets need to be in branches). So I'm doing 
		//this every tick at the moment. Perhaps a different, more clever system, could be created but then again, for what gain? This is a model editor.
		//and this project has went on for over a year now. In an effort to finish this up, just tick!
		if (levelCamera && activeConfig)
		{
			levelCamera->updateTrackedLevelCollision(*activeConfig);
		}
	}

	void ModelConfigurerEditor_Level::handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx)
	{
		player->getInput().onKey.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleKey);
		player->getInput().onButton.addWeakObj(sp_this(), &ModelConfigurerEditor_Level::handleMouseButton);
		if (const sp<CameraBase>& camera = player->getCamera())
		{
			//configure camera to look at model
			camera->setPosition({ 5, 0, 0 });
			camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
			camera->setFar(1000.f);
			cachedPlayerCamera = camera;
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
		activeConfig = nullptr;
	}

	void ModelConfigurerEditor_Level::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		updateCameras();
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
							activeConfig = kvPair.second;
							selectedSpawnConfigIdx = curConfigIdx;

							onActiveConfigSet(*activeConfig);
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
				activeConfig->save();
			}
			/////////////////////////////////////////////////
			// Button Delete
			/////////////////////////////////////////////////
			if (activeConfig)
			{
				if (activeConfig->isDeletable())
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
				if (activeConfig)
				{
					ImGui::Text("DELETE:"); ImGui::SameLine(); ImGui::Text(activeConfig->getName().c_str());
					ImGui::Text("WARNING: this operation is irreversible!");
					ImGui::Text("Do you really want to delete this spawn config?");

					if (ImGui::Button("DELETE", ImVec2(120, 0)))
					{
						if (activeMod)
						{
							activeMod->deleteSpawnConfig(activeConfig);
							activeConfig = nullptr;
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

		if (activeConfig)
		{
			////////////////////////////////////////////////////////
			// Model Transform
			////////////////////////////////////////////////////////
			ImGui::Text("Model Transform Configuration");
			ImGui::InputFloat3("Scale", &activeConfig->modelScale.x);
			ImGui::InputFloat3("Rotation", &activeConfig->modelRotationDegrees.x);
			ImGui::InputFloat3("Translation", &activeConfig->modelPosition.x);
			ImGui::Dummy(ImVec2(0, 20));
			////////////////////////////////////////////////////////
			// use collision tests
			////////////////////////////////////////////////////////
			ImGui::Checkbox("Request collision tests against this object", &activeConfig->bRequestsCollisionTests);
			ImGui::Dummy(ImVec2(0, 20));
			////////////////////////////////////////////////////////
			// AABB
			////////////////////////////////////////////////////////
			ImGui::Checkbox("Show Model AABB collision", &bRenderAABB);
			//ImGui::Checkbox("Use Model AABB pretest optimization", &activeConfig->bUseModelAABBTest);
			ImGui::Dummy(ImVec2(0, 20));

			////////////////////////////////////////////////////////
			// True Collision Shapes
			////////////////////////////////////////////////////////
			if (ImGui::Button("Add Shape"))
			{
				activeConfig->shapes.push_back({});
			}

			if (selectedShapeIdx != -1)
			{
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					activeConfig->shapes.erase(activeConfig->shapes.begin() + selectedShapeIdx);
					selectedShapeIdx = -1;
				}
			}

			unsigned int shapeIdx = 0;
			ImGuiWindowFlags file_load_wndflags = ImGuiWindowFlags_HorizontalScrollbar;

			ImGui::Separator();
			for (CollisionShapeSubConfig& shapeConfig : activeConfig->shapes)
			{
				if (shapeConfig.shape == int(ECollisionShape::MODEL))
				{
					size_t lastSlashIndex = shapeConfig.modelFilePath.find_last_of("/");
					if (lastSlashIndex == std::string::npos)
					{
						lastSlashIndex = shapeConfig.modelFilePath.find_last_of("\\");
					}
					std::string modelFileName = "";
					if (lastSlashIndex != std::string::npos
						&& lastSlashIndex != shapeConfig.modelFilePath.size())
					{
						modelFileName = shapeConfig.modelFilePath.substr(lastSlashIndex);
					}
					if (modelFileName.length() == 0)
					{
						modelFileName = "NA";
					}

					snprintf(tempTextBuffer, sizeof(tempTextBuffer), "shape %d : %s - %s",
						shapeIdx,
						shapeToStr((ECollisionShape)shapeConfig.shape),
						modelFileName.c_str()
						);
				}
				else
				{
					snprintf(tempTextBuffer, sizeof(tempTextBuffer), "shape %d : %s", shapeIdx, shapeToStr((ECollisionShape) shapeConfig.shape));
				}
				if (ImGui::Selectable(tempTextBuffer, shapeIdx == selectedShapeIdx))
				{
					if (selectedShapeIdx != shapeIdx)
					{
						selectedShapeIdx = shapeIdx;
					}
					else
					{
						selectedShapeIdx = -1;
					}
				}
				if (shapeIdx == selectedShapeIdx)
				{
					if (selectedShapeIdx >= 0 && selectedShapeIdx < (int)activeConfig->shapes.size())
					{
						if (activeConfig->shapes[selectedShapeIdx].shape == int(ECollisionShape::MODEL))
						{
							ImGui::Text("Model Path: "); 
							ImGui::SameLine();

							constexpr size_t fileLengthLimit = 1024;
							static char collisionModelFilePath[fileLengthLimit + 1];
							static int lastIndex = -1;
							if (collisionModelFilePath[0] == 0 || lastIndex != selectedShapeIdx)
							{
								lastIndex = selectedShapeIdx;
								std::string loadedModelFilePath = activeConfig->shapes[selectedShapeIdx].modelFilePath;

								if (loadedModelFilePath.length() > fileLengthLimit)
								{
									log(__FUNCTION__, LogLevel::LOG, "loaded file length of collision model too large, trimming");
									loadedModelFilePath = loadedModelFilePath.substr(0, fileLengthLimit - 1); //-1 because I'm going quick atm, probably can be just limit; but better safe than sorry
									activeConfig->shapes[selectedShapeIdx].modelFilePath = loadedModelFilePath;
								}
								else if(loadedModelFilePath.length() == 0)
								{ 
									std::memset(&collisionModelFilePath, '\0', sizeof(char)*fileLengthLimit);
									loadedModelFilePath = "no_filepath";
								}
								std::memcpy(collisionModelFilePath, loadedModelFilePath.c_str(), loadedModelFilePath.size());
							}

							ImGui::InputText("Collision Model Path", collisionModelFilePath, fileLengthLimit, ImGuiInputTextFlags_CharsNoBlank);
							if (ImGui::Button("Load Collision Model"))
							{
								tryLoadCollisionModel(collisionModelFilePath);
								collisionModelFilePath[0] = 0; //make length 0 so it will read the actual model file path
							}
						}

						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Scale Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].scale.x);

						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Rotation Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].rotationDegrees.x);

						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Translation Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].position.x);

						ImGui::RadioButton("Cube (3e 3f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::CUBE);
						ImGui::SameLine(); ImGui::RadioButton("PolyCapsule (7e 6f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::POLYCAPSULE);
						ImGui::SameLine(); ImGui::RadioButton("model", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::MODEL);
						if (bShowCustomShapes)
						{
							ImGui::RadioButton("Wedge (12e 5f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::WEDGE);
							ImGui::SameLine(); ImGui::RadioButton("Pyramid (8e 5f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::PYRAMID);
							if (bShowSlowShapes)
							{
								ImGui::RadioButton("Icosphere (15e 10f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::ICOSPHERE);
								ImGui::SameLine(); ImGui::RadioButton("UVSphere (36e 16f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::UVSPHERE);
							}
						}
					}
				}
				++shapeIdx;
			}
			ImGui::Dummy(ImVec2(0, 20));
			ImGui::Checkbox("Show Custom Shapes", &bShowCustomShapes);
			ImGui::SameLine(); ImGui::Checkbox("Show Slow Shapes", &bShowSlowShapes);
			ImGui::Checkbox("Render Collision Shapes", &bRenderCollisionShapes);
			ImGui::Checkbox("Render With Lines", &bRenderCollisionShapesLines);

			ImGui::Dummy(ImVec2(0, 20));
			if (ImGui::TreeNode("What's (3e 3f)?"))
			{
				const char* msg_3f3e = R"(
The collision uses the "Separating Axis Theorem" (SAT) to determine if two objects are colliding. Without going into the specifics, SAT can test if two shapes are colliding via a set of tests. These tests involve the shape's faces and edges between faces. 3e 3f means this shape as 3 edges and 3 faces
)";
				ImGui::TextWrapped(msg_3f3e);
				ImGui::Dummy(ImVec2(0, 20));

				const char* msg_howtouse = R"(
How is this useful? Collision is an expensive test. SAT works by generating a set axes (think a group of lines). It then casts shadows of the shapes onto the axes to see if there is any place where the shape's shadows doesn't overlap. If there exists a place where the shadow's don't overlap, then the shapes are not colliding. Where the 3e 3f is relevant is in how those axes are generated. Each face has an axis following its normal. Each edge from shape 'A' must be combined with every edge in the other shape 'B' to generate an axis (for the math junkies, the axis is the result of a cross product). Casting A's shadow onto a single axis is also a slow operation; each vertex in 'A' must be projected onto the axis. So, the less axes needing testing the better!
)";
				ImGui::TextWrapped(msg_howtouse);
				ImGui::Dummy(ImVec2(0, 20));

				const char* msg_example = R"(
As an example, say we do a collision test between two cubes, A and B; that is testing (3e3f) with (3e3f). Since each cube has 3 faces, that's 6 total axes from the faces (3 + 3). However, we must multiply the edges together, not add. So that is 3e * 3e = 9 axes from the edges. That totals 15 axes to test (6 + 9). So, it is much better to have less edges. Testing a pyramid(8e5f) with a cube(3e3f) results in 8e*3e + 5f+3f = 24axes + 8axes = 32axes; yikes. So, prefer cubes if possible!
)";
				ImGui::TextWrapped(msg_example);
				ImGui::Dummy(ImVec2(0, 20));

				const char* msg_whatShouldIDo = R"(
So, what should you do? Well: 1. Uses as efficient shapes as possible. 2. Use as few shapes as possible. In regards to point 2: if each model has 3 collision shapes, then that is 3x3=9 tests that need to happen. Each of those tests entails what was described above. There is a general pre-test that uses 2 cubes (AABB pretest optimization) that does a quick filter at the expense of 2 cube tests. But if that test fails, then all the collision shapes will be tested. Which means you may see drops in framerate when objects get really close to colliding if you use a lot of shapes.
)";
				ImGui::TextWrapped(msg_whatShouldIDo);
				ImGui::TreePop();
			}

			ImGui::Dummy(ImVec2(0, 20));

		}
		
		ImGui::Separator();
		ImGui::Checkbox("Render Avoidance Spheres", &bRenderAvoidanceSpheres);
		if (ImGui::Button("Add avoidance sphere"))
		{
			activeConfig->avoidanceSpheres.push_back({});
		}

		if (selectedAvoidanceSphereIdx != -1)
		{
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
			{
				activeConfig->avoidanceSpheres.erase(activeConfig->avoidanceSpheres.begin() + selectedAvoidanceSphereIdx);
				selectedShapeIdx = -1;
			}
		}
		int avoidanceIndex = 0;
		for (AvoidanceSphereSubConfig& avoidSphereConfig : activeConfig->avoidanceSpheres)
		{
			snprintf(tempTextBuffer, sizeof(tempTextBuffer), "sphere %d : %f radius", avoidanceIndex, avoidSphereConfig.radius);
			if (ImGui::Selectable(tempTextBuffer, avoidanceIndex == selectedAvoidanceSphereIdx))
			{
				//enter here if clicked the selectable
				if (selectedAvoidanceSphereIdx != avoidanceIndex)
				{
					selectedAvoidanceSphereIdx = avoidanceIndex;
				}
				else
				{
					selectedAvoidanceSphereIdx = -1;
				}
			}
			avoidanceIndex++;
		}
		if (selectedAvoidanceSphereIdx != -1)
		{
			ImGui::InputFloat3("position", &activeConfig->avoidanceSpheres[selectedAvoidanceSphereIdx].localPosition.x);
			ImGui::InputFloat("radius", &activeConfig->avoidanceSpheres[selectedAvoidanceSphereIdx].radius);
		}

		ImGui::Dummy(ImVec2(0, 20));
		ImGui::Separator();


		//pretty terrible, but doing this every tick after UI has had change to update. This isn't so bad because this is just a model editor.
		//but it will have performance issues when the model grows more complex and has many collision shapes. But that would be bad for 
		//game performance, so this can be used as a proxy to know if the user is creating a model that will be bad for performance.
		//-- the alternative to doing this every tick is to have each ui element above that potentially changes collision cause a rebuild.
		//that's a lot of work for little gain at this point (it also muddys up the UI code as all widgets need to be in branches). So I'm doing 
		//this every tick at the moment. Perhaps a different, more clever system, could be created but then again, for what gain? This is a model editor.
		//and this project has went on for over a year now. In an effort to finish this up, just tick!
		if (levelCamera && activeConfig)
		{
			levelCamera->updateTrackedLevelCollision(*activeConfig);
		}
	}

	void ModelConfigurerEditor_Level::renderUI_Projectiles()
	{
		ImGui::Separator(); //separate out this section of UI

		static sp<ModSystem> modSys = SpaceArcade::get().getModSystem();

		if (const sp<Mod>& activeMod = modSys->getActiveMod())
		{
			ImGui::Text("Primary Fire Projectile");
			const std::map<std::string, sp<ProjectileConfig>>& projectileConfigs = activeMod->getProjectileConfigs();

			for (const auto& kvPair : projectileConfigs)
			{
				if (ImGui::Selectable(kvPair.first.c_str(), kvPair.second == primaryProjectileConfig))
				{
					primaryProjectileConfig = kvPair.second;

					if (activeConfig)
					{
						activeConfig->primaryFireProjectile = primaryProjectileConfig;
					}
				}
			}
			ImGui::Dummy({ 0, 10.f });
		}

		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::updateCameraSpeed()
	{
		const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
		if (zeroPlayer)
		{
			float adjustedCameraSpeed = cameraSpeedModifier * 10;

			//perhaps camera speed should be a concept of the base class.
			CameraBase* rawCamera = zeroPlayer->getCamera().get();
			if (CameraFPS* camera = dynamic_cast<CameraFPS*>(rawCamera))
			{
				camera->setSpeed(adjustedCameraSpeed);
			}
			else if (QuaternionCamera* camera = dynamic_cast<QuaternionCamera*>(rawCamera))
			{
				camera->setSpeed(adjustedCameraSpeed);
			}
		}
	}

	void ModelConfigurerEditor_Level::renderUI_ViewportUI()
	{
		ImGui::Separator();
		if (ImGui::SliderFloat("Camera Speed", &cameraSpeedModifier, 1.0f, 10.f))
		{
			updateCameraSpeed();
		}
		ImGui::Checkbox("Model X-ray", &bModelXray);
		ImGui::SameLine();
		if (ImGui::Checkbox("use collision camera", &bUseCollisionCamera))
		{
			updateCameras();
		}
		//DEBUG memory leak
		ImGui::SameLine();
		ImGui::Checkbox("collision tick", &bEnableCollisionTick);
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Team()
	{
		char strBuffer[256];
		constexpr size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);

		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();

		ImGui::Separator();
		if (activeConfig && activeMod)
		{
			////////////////////////////////////////////////////////
			// controlling default configs for mod
			////////////////////////////////////////////////////////
			if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
			{
				ImGui::Text("Mods need to look up what should be the default carrier for a given team so that it knows what to spawn for some gamemodes.");
				ImGui::Text("If the config name is changed, this data will need to be re-applied.");

				static int carrierTeamIDProxy = 0;
				ImGui::SliderInt("Team to make this the default carrier (multiple acceptable)", &carrierTeamIDProxy, 0, MAX_TEAM_NUM-1);

				if(ImGui::Button("Make this default carrier for team"))
				{
					activeMod->setDeafultCarrierConfigForTeam(activeConfig->getName(), carrierTeamIDProxy);
				}
				if(ImGui::Button("Remove default carrier for this index"))
				{
					activeMod->setDeafultCarrierConfigForTeam("",carrierTeamIDProxy);
				}
				if(ImGui::Button("Save To Mod"))
				{
					activeMod->writeToFile();
				}
			}

			////////////////////////////////////////////////////////
			// Individual team data
			////////////////////////////////////////////////////////

			if (activeConfig->teamData.size() == 0)
			{
				activeConfig->teamData.push_back({});
			}

			for (size_t teamIdx = 0; teamIdx < activeConfig->teamData.size(); ++teamIdx)
			{
				TeamData& teamData = activeConfig->teamData[teamIdx];
				if (activeMod->teamHasName(teamIdx))
				{
					std::string teamName = activeMod->getTeamName(teamIdx);
					snprintf(strBuffer, strBufferSize, teamName.c_str(), teamIdx);
				}
				else
				{
					snprintf(strBuffer, strBufferSize, "Team %d", teamIdx);
				}
				if (ImGui::CollapsingHeader(strBuffer))
				{
					snprintf(strBuffer, strBufferSize, "Team Tint %d", teamIdx);
					ImGui::ColorPicker3(strBuffer, &teamData.teamTint.r);
					ImGui::Dummy(ImVec2(0, 20)); 

					snprintf(strBuffer, strBufferSize, "Shield Color %d", teamIdx);
					ImGui::ColorPicker3(strBuffer, &teamData.shieldColor.r);
					ImGui::Dummy(ImVec2(0, 20)); 

					snprintf(strBuffer, strBufferSize, "projectile Color %d", teamIdx);
					ImGui::ColorPicker3(strBuffer, &teamData.projectileColor.r);
					ImGui::Dummy(ImVec2(0, 20)); //bottom spacing

					static char inputNameBuffer[2048];
					ImGui::InputText("Team Name", inputNameBuffer, sizeof(inputNameBuffer));
					if (ImGui::Button("Commit Team Name"))
					{
						activeMod->setTeamName(teamIdx, inputNameBuffer);
						activeMod->writeToFile();
					}
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Add Team"))
			{
				activeConfig->teamData.push_back({});
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove Last"))
			{
				activeConfig->teamData.pop_back();
			}
			static int swapIndices[2] = { 0, 0 };
			ImGui::InputInt2("[0,size-1]", swapIndices);
			ImGui::SameLine();
			if (ImGui::Button("Swap"))
			{
				int limit = (int)activeConfig->teamData.size(); //realistically there's not chance of overflow; cast is safe
				if (swapIndices[0] < limit && swapIndices[0] >= 0
					&& swapIndices[1] < limit && swapIndices[1] >= 0
					&& swapIndices[0] != swapIndices[1])
				{
					TeamData copyFirst = activeConfig->teamData[size_t(swapIndices[0])];
					activeConfig->teamData[size_t(swapIndices[0])] = activeConfig->teamData[size_t(swapIndices[1])];
					activeConfig->teamData[size_t(swapIndices[1])] = copyFirst;
				}
			}
			static int viewTeamIdx = 0;
			ImGui::InputInt("View Team [0,size-1]", &viewTeamIdx);
			
			//always set state for real time updates.
			if (viewTeamIdx < (int)activeConfig->teamData.size() && viewTeamIdx >= 0)
			{
				activeTeamData = activeConfig->teamData[viewTeamIdx];
			}

		}
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Objectives()
	{
		ImGui::Separator();
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeConfig && activeMod)
		{
			auto renderList = [this](
				const char* objectNameWithIndex,
				std::vector<PlacementSubConfig>& placementSubConfigArray,
				int& selectedPlacementIndex,
				const char* newPlacementButtonStr,
				const char* deletePlaceementStr,
				std::vector<sp<ShipPlacementEntity>>& outModelPlacement,
				const sp<Mod>& activeMod,
				PlacementType placementType
				)
			{
				constexpr size_t bufferPathLength = 2048;
				static char filepathBuffer[bufferPathLength + 1];

				int placementIterIndex = 0;

				bool bRefreshFileDir = false;
				for (PlacementSubConfig& commConfig : placementSubConfigArray)
				{
					snprintf(tempTextBuffer, sizeof(tempTextBuffer), objectNameWithIndex, placementIterIndex);
					if (ImGui::Selectable(tempTextBuffer, placementIterIndex == selectedPlacementIndex))
					{
						bRefreshFileDir = true;

						//enter here if clicked the selectable
						if (selectedPlacementIndex != placementIterIndex)
						{
							//clear all other selections since they share a filepath buffer
							selectedTurretPlacementIdx = -1;
							selectedPlacementIndex = -1;
							selectedCommPlacementIdx = -1;

							selectedPlacementIndex = placementIterIndex;

							//clear the buffer so tail end chars are not still in buffer when previously selecting long paths
							memset(filepathBuffer, 0, sizeof(filepathBuffer));
						}
						else
						{
							selectedPlacementIndex = -1;
						}
					}
					placementIterIndex++;
				}
				static std::string defaultpath = "Assets/Models3D/satellite/GroundSatellite.obj";
				if (selectedPlacementIndex != -1 &&  selectedPlacementIndex < int(placementSubConfigArray.size()))
				{
					bool bRefreshModelMat = false;
					if (ImGui::InputFloat3("localPosition", &placementSubConfigArray[selectedPlacementIndex].position.x)) { bRefreshModelMat = true; };
					if (ImGui::InputFloat3("localRotation", &placementSubConfigArray[selectedPlacementIndex].rotation_deg.x)) { bRefreshModelMat = true; };
					if (ImGui::InputFloat3("localScale", &placementSubConfigArray[selectedPlacementIndex].scale.x)) { bRefreshModelMat = true; };

					if (bRefreshModelMat)
					{
						//force update of preview entity
						Transform newXform;
						newXform.position = placementSubConfigArray[selectedPlacementIndex].position;
						newXform.rotQuat = Utils::degreesVecToQuat(placementSubConfigArray[selectedPlacementIndex].rotation_deg);
						newXform.scale = placementSubConfigArray[selectedPlacementIndex].scale;
						outModelPlacement[selectedPlacementIndex]->setTransform(newXform);
					}


					std::string& loadedPath = placementSubConfigArray[selectedPlacementIndex].relativeFilePath;


					std::string modPath = activeMod->getModDirectoryPath();
					size_t startModPathIdx = loadedPath.find(modPath);
					if (startModPathIdx != std::string::npos)
					{
						loadedPath.erase(startModPathIdx, modPath.length());
					}

					if (loadedPath.length() == 0) 
					{ 
						std::memcpy(filepathBuffer, defaultpath.c_str(), defaultpath.length());
					}
					else if (bRefreshFileDir)
					{
						std::memcpy(filepathBuffer, loadedPath.c_str(), glm::min<size_t>(loadedPath.size(), bufferPathLength)); 
					}

					ImGui::InputText("3D model Filepath", filepathBuffer, bufferPathLength, 0);
					if (ImGui::Button("Refresh Placement Model"))
					{
						placementSubConfigArray[selectedPlacementIndex].relativeFilePath = filepathBuffer;
						std::string fullPath = placementSubConfigArray[selectedPlacementIndex].getFullPath(*activeConfig);
						sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(fullPath.c_str());
						outModelPlacement[selectedPlacementIndex]->replaceModel(model);
						if (model) { defaultpath = filepathBuffer;}
					}
				}
				if (ImGui::Button(newPlacementButtonStr))
				{
					std::string thisDefaultModelPath = activeMod->getModDirectoryPath() + defaultpath;

					placementSubConfigArray.push_back(PlacementSubConfig{});
					PlacementSubConfig& newPlacement = placementSubConfigArray.back();
					newPlacement.relativeFilePath = thisDefaultModelPath;
					newPlacement.placementType = placementType;
					if (selectedPlacementIndex != -1)
					{
						//copy previous for easy tweaking
						PlacementSubConfig& previousPlacement = placementSubConfigArray[selectedPlacementIndex];
						newPlacement.position = previousPlacement.position;
						newPlacement.scale = previousPlacement.scale;
						newPlacement.rotation_deg = previousPlacement.rotation_deg;
					}

					outModelPlacement.push_back(new_sp<ShipPlacementEntity>());

					if (sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(thisDefaultModelPath.c_str())) //this may fail on custom mods
					{
						outModelPlacement.back()->replaceModel(model);	
					};
				}
				if (selectedPlacementIndex != -1 && ImGui::Button(deletePlaceementStr))
				{
					placementSubConfigArray.erase(placementSubConfigArray.begin() + selectedPlacementIndex);
					outModelPlacement.erase(outModelPlacement.begin() + selectedPlacementIndex);
					selectedPlacementIndex = -1; //clear index after removal
				}
			};
			renderList("Communication Object %d", activeConfig->communicationPlacements, selectedCommPlacementIdx, "New Communication Placement", "delete Comm", placement_communications, activeMod, PlacementType::COMMUNICATIONS);
			ImGui::Separator();
			renderList("Turret Object %d", activeConfig->turretPlacements, selectedTurretPlacementIdx, "New Turret Placement", "delete turret", placement_turrets, activeMod, PlacementType::TURRET);
			ImGui::Separator();
			renderList("DefenseObject %d", activeConfig->defensePlacements, selectedDefensePlacementIdx, "New Defense Placement", "delete defense", placement_defenses, activeMod, PlacementType::DEFENSE);
			ImGui::Dummy(ImVec2(0, 20));
		}
	}

	void ModelConfigurerEditor_Level::renderUI_Spawning()
	{
		ImGui::Separator();
		if (activeConfig)
		{
			ImGui::Text("Fighter configs to spawn (displayed trailing indices are ignored)");
			for (int spawnableConfigIdx = 0; spawnableConfigIdx < int(activeConfig->spawnableConfigsByName.size()); ++spawnableConfigIdx)
			{
				snprintf(tempTextBuffer, sizeof(tempTextBuffer), "%s %d", activeConfig->spawnableConfigsByName[spawnableConfigIdx].c_str(), spawnableConfigIdx);
				if (ImGui::Selectable(tempTextBuffer, spawnableConfigIdx == selectedSpawnableConfigNameIdx))
				{
					//select new name to edit
					selectedSpawnableConfigNameIdx = spawnableConfigIdx;
				} 
			}

			if (selectedSpawnableConfigNameIdx >= 0 && selectedSpawnableConfigNameIdx < int(activeConfig->spawnableConfigsByName.size()))
			{
				static char configName[4096];
				ImGui::InputText("spawn config name", configName, sizeof(configName));
				if (ImGui::Button("commit name"))
				{
					activeConfig->spawnableConfigsByName[selectedSpawnableConfigNameIdx] = configName;
				}
			}
			if (ImGui::Button("new spawnable config"))
			{
				activeConfig->spawnableConfigsByName.push_back("");
			}
			if (selectedSpawnableConfigNameIdx >= 0 && selectedSpawnableConfigNameIdx < int(activeConfig->spawnableConfigsByName.size()))
			{
				ImGui::SameLine();
				if (ImGui::Button("Delete Spawn Point"))
				{
					activeConfig->spawnableConfigsByName.erase(activeConfig->spawnableConfigsByName.begin() + selectedSpawnableConfigNameIdx);
					selectedSpawnableConfigNameIdx = -1; //clear index after removal
				}
			}

			ImGui::Separator();
			for (int spawnPointIdx = 0; spawnPointIdx < int(activeConfig->spawnPoints.size()); ++spawnPointIdx)
			{
				snprintf(tempTextBuffer, sizeof(tempTextBuffer), "spawn point %d", spawnPointIdx);
				if (ImGui::Selectable(tempTextBuffer, spawnPointIdx == selectedSpawnPointIdx))
				{
					selectedSpawnPointIdx = spawnPointIdx;
				}
			}
			if (selectedSpawnPointIdx >= 0 && selectedSpawnPointIdx < int(activeConfig->spawnPoints.size()))
			{
				FighterSpawnPoint& spawn = activeConfig->spawnPoints[selectedSpawnPointIdx];
				ImGui::DragFloat3("local pos", &spawn.location_lp.x, 0.05f);
				ImGui::DragFloat3("direction", &spawn.direction_ln.x, 0.05f);
				ImGui::Text("Note: the direction will be normalized when saving");
			}
			if (ImGui::Button("New spawn point"))
			{
				activeConfig->spawnPoints.push_back(FighterSpawnPoint{});
			}
			if (selectedSpawnPointIdx != -1 && selectedSpawnPointIdx < int(activeConfig->spawnPoints.size()))
			{
				ImGui::SameLine();
				if (ImGui::Button("Delete Spawn Point"))
				{
					activeConfig->spawnPoints.erase(activeConfig->spawnPoints.begin() + selectedSpawnPointIdx);
					selectedSpawnPointIdx = -1; //clear index after removal
				}
			}
		}
		else
		{
			ImGui::Text("Select a ship config");
		}
		ImGui::Dummy(ImVec2(0, 20));
	}

	void ModelConfigurerEditor_Level::renderUI_Sounds()
	{
		ImGui::Separator();

		if (activeConfig)
		{

#define SFX_UI(name_cstr, sfx_getter, sfx_setter, text_temp_buffer)\
			{\
				static SoundEffectSubConfig sfxConfig = activeConfig->sfx_getter(); /*make a copy and use this to track data*/\
				static int oneTimeInit = [this](const SoundEffectSubConfig& sfxConfig){ snprintf(text_temp_buffer, sizeof(text_temp_buffer), "%s", sfxConfig.assetPath.c_str()); return 0;}(sfxConfig);\
				ImGui::Text(name_cstr " SFX:");\
				ImGui::SameLine();\
				ImGui::Text(sfxConfig.assetPath.c_str());\
				ImGui::InputText(name_cstr " SFX Mod Relative Path", text_temp_buffer, sizeof(text_temp_buffer));\
				ImGui::InputFloat("maxDistance " name_cstr , &sfxConfig.maxDistance);\
				ImGui::InputFloat("volume " name_cstr , &sfxConfig.gain);\
				ImGui::InputFloat("pitch varation" name_cstr , &sfxConfig.pitchVariationRange);\
				ImGui::Checkbox("bLooping " name_cstr , &sfxConfig.bLooping);\
				static bool bPlayWindowProxy = sfxConfig.oneshotTimeoutSec.has_value();\
				ImGui::Checkbox("fail to play time window" name_cstr, &bPlayWindowProxy);\
				if(bPlayWindowProxy)\
				{\
					float timeProxy = sfxConfig.oneshotTimeoutSec.has_value() ? *sfxConfig.oneshotTimeoutSec : 0.f;\
					if (ImGui::InputFloat("try To Play For X Seconds " name_cstr, &timeProxy))\
					{\
						sfxConfig.oneshotTimeoutSec = timeProxy; \
					}\
				}\
				if (ImGui::Button("save sound: " name_cstr ))\
				{\
					sfxConfig.assetPath = text_temp_buffer;\
					activeConfig->sfx_setter(sfxConfig);\
					sfxConfig = activeConfig->sfx_getter();/*refresh our static copy*/\
					activeConfig->save();\
				}\
				ImGui::Separator();\
			}

			//NOTE: these must have unique names_cstr for IMGUI to work correctly!
			SFX_UI("Engine Loop", getConfig_sfx_engineLoop, setConfig_sfx_engineLoop, engineSoundPathName);
			SFX_UI("Projectile Loop", getConfig_sfx_projectileLoop, setConfig_sfx_projectileLoop, projectileSoundPathName);
			SFX_UI("Explosion", getConfig_sfx_explosion, setConfig_sfx_explosion, explosionSoundPathName);
			SFX_UI("Muzzle Sound", getConfig_sfx_muzzle, setConfig_sfx_muzzle, muzzleSoundPathName);

		}
	}

	void ModelConfigurerEditor_Level::createNewSpawnConfig(const std::string& configName, const std::string& fullModelPath)
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			renderModel = SpaceArcade::get().getAssetSystem().loadModel(fullModelPath.c_str());

			sp<SpawnConfig> newConfig = new_sp<SpawnConfig>();
			newConfig->fileName = configName;
			newConfig->fullModelFilePath = fullModelPath;
			newConfig->owningModDir = activeMod->getModDirectoryPath();
			newConfig->save();

			activeMod->addSpawnConfig(newConfig);

			activeConfig = newConfig;
		}
	}

	void ModelConfigurerEditor_Level::onActiveConfigSet(const SpawnConfig& newConfig)
	{
		std::string modelPath = activeConfig->getModelFilePath();

		//may fail to load model if user provided bad path, either way we need to set to null or valid model
		renderModel = SpaceArcade::get().getAssetSystem().loadModel(modelPath.c_str());

		primaryProjectileConfig = activeConfig->getPrimaryProjectileConfig();

		//LOAD SHIP TAKEDOWN OBJECTIVES
		auto loadPlacements = [this](
				const std::vector<PlacementSubConfig>& inPlacementConfigs,
				std::vector<sp<ShipPlacementEntity>>& outEntities,
				ConfigBase& activeCFG
			)
		{
			outEntities.clear();
			for (const PlacementSubConfig& placement : inPlacementConfigs)
			{
				sp<ShipPlacementEntity> newPlacement = new_sp<ShipPlacementEntity>();
				Transform xform;
				xform.position = placement.position;
				xform.rotQuat = Utils::degreesVecToQuat(placement.rotation_deg);
				xform.scale = placement.scale;
				newPlacement->setTransform(xform);
				if (placement.relativeFilePath.length() > 0)
				{
					newPlacement->replaceModel(GameBase::get().getAssetSystem().loadModel(placement.getFullPath(activeCFG)));
				}
				outEntities.push_back(newPlacement);
			}
		};
		if (activeConfig)
		{
			loadPlacements(activeConfig->defensePlacements, placement_defenses, *activeConfig);
			loadPlacements(activeConfig->turretPlacements, placement_turrets, *activeConfig);
			loadPlacements(activeConfig->communicationPlacements, placement_communications, *activeConfig);
		}
	}

	void ModelConfigurerEditor_Level::tryLoadCollisionModel(const char* filePath)
	{
		//try load even if a file is already in the map. This way we can do file refreshes while running.
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod)
		{
			try
			{
				std::string fullRelativeAssetPath = activeMod->getModDirectoryPath() + filePath;

				sp<Model3D> newModel = new_sp<Model3D>(fullRelativeAssetPath.c_str());
				if (newModel)
				{
					TriangleProcessor processedModel = modelToCollisionTriangles(*newModel);
					sp<SAT::Shape> modelCollision = new_sp<SAT::DynamicTriangleMeshShape>(processedModel);

					std::string filePathAsStr{ filePath };
					collisionModels[filePathAsStr] = newModel;
					//collisionModelSATShapes[filePathAsStr] = modelCollision;
					collisionModelSATShapesRenders[filePathAsStr] = new_sp<ShapeRenderWrapper>(modelCollision);

					if (selectedShapeIdx >= 0 && size_t(selectedShapeIdx) < activeConfig->shapes.size())
					{
						activeConfig->shapes[size_t(selectedShapeIdx)].modelFilePath = std::string(filePath);
					}

					return; //early out so failure log isn't printed at end of this function.
				}
			}
			catch (...)
			{}
		}
		log(__FUNCTION__, LogLevel::LOG, "Failed to load collision model");
	}

	void ModelConfigurerEditor_Level::updateCameras()
	{
		if (const sp<SA::PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			sp<CameraBase> from = nullptr;
			sp<CameraBase> to = nullptr;
			if (bUseCollisionCamera)
			{
				from = cachedPlayerCamera;
				to = levelCamera;
			}
			else
			{
				to = cachedPlayerCamera;
				from = levelCamera;
			}
			if (from && to)
			{
				to->setPosition(from->getPosition());
				to->lookAt_v(from->getPosition() + from->getFront());
				player->setCamera(to);
				updateCameraSpeed();

				if (const sp<Window>& window = GameBase::get().getWindowSystem().getPrimaryWindow())
				{
					to->registerToWindowCallbacks_v(window);
					from->deregisterToWindowCallbacks_v();
				}
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "missing camera");
			}
		}
	}

	void ModelConfigurerEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using namespace glm;

		//#TODO refactor this to use the CollisionDebugRenderer class

		if (renderModel && shapeRenderer && activeConfig)
		{
			Transform rootXform = activeConfig->getModelXform();
			mat4 rootModelMat = rootXform.getModelMatrix();

			const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
			if (zeroPlayer)
			{
				const sp<CameraBase>& camera = zeroPlayer->getCamera();

				shapeRenderer->renderAxes(mat4(1.f), view, projection);
				
				{ //render model
					model3DShader->use();
					model3DShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
					model3DShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
					model3DShader->setUniform3f("cameraPosition", camera->getPosition());
					model3DShader->setUniform3f("tint", activeTeamData.teamTint);
					model3DShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(rootModelMat));
					if (bModelXray){glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);}
					renderModel->draw(*model3DShader);
					if (bModelXray) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
				}

				if (bRenderAABB)
				{
					std::tuple<vec3, vec3> aabbRange = renderModel->getAABB();
					vec3 aabbMinToMax = std::get<1>(aabbRange) - std::get<0>(aabbRange); //max - min

					//correct for model center mis-alignments; this should be cached in game so it isn't calculated each frame
					vec3 aabbCenterPnt = std::get</*min*/0>(aabbRange) + (0.5f * aabbMinToMax);
					//we can now use aabbCenter as a translation vector for the aabb!

					mat4 aabbModel = glm::translate(rootModelMat, aabbCenterPnt);
					aabbModel = glm::scale(aabbModel, aabbMinToMax);

					shapeRenderer->renderUnitCube({ aabbModel, view, projection, glm::vec3(0,0,1), GL_LINE, GL_FILL });
				}

				if (bRenderAvoidanceSpheres)
				{
					for (AvoidanceSphereSubConfig& avSphereCfg : activeConfig->avoidanceSpheres)
					{
						sharedAvoidanceRenderer->setRadius(avSphereCfg.radius);
						sharedAvoidanceRenderer->setPosition(avSphereCfg.localPosition);
						sharedAvoidanceRenderer->render();
					}
				}

				if (bool bRenderObjectives = true)
				{
					static const auto renderPlacement = [](
						const std::vector<sp<ShipPlacementEntity>>& placementContainer,
						const sp<Shader>& model3DShader,
						bool bModelXray)
					{
						if (model3DShader)
						{
							for (const sp<ShipPlacementEntity>& placement : placementContainer)
							{
								if (bModelXray) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }

								model3DShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(placement->getParentXLocalModelMatrix()));
								placement->render(*model3DShader);

								if (bModelXray) { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
							}
						}
					};
					//this code is relying on the fact that the shader was configured previously for rendering the main model.
					renderPlacement(placement_communications, model3DShader, bModelXray);
					renderPlacement(placement_defenses, model3DShader, bModelXray);
					renderPlacement(placement_turrets, model3DShader, bModelXray);
				}

				if (bool bRenderActivePlacementBox = true)
				{
					auto renderBoxAtPlacement = [](
							int idx, const std::vector<sp<ShipPlacementEntity>>& placement_container
						) 
					{
						if (idx >= 0 && (idx < int(placement_container.size())) )
						{
							if (const sp<ShipPlacementEntity>& placement = placement_container[idx])
							{
								DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
								debugRenderSystem.renderCube(placement->getParentXLocalModelMatrix(), SA::color::metalicgold());
							}
						}
					};
					renderBoxAtPlacement(selectedCommPlacementIdx, placement_communications);
					renderBoxAtPlacement(selectedDefensePlacementIdx, placement_defenses);
					renderBoxAtPlacement(selectedTurretPlacementIdx, placement_turrets);
				}

				if (bRenderCollisionShapes)
				{
					GLenum renderMode = bRenderCollisionShapesLines ? GL_LINE : GL_FILL;
					ec(glPolygonMode(GL_FRONT_AND_BACK, renderMode)); 

					int shapeIdx = 0;
					for (CollisionShapeSubConfig shape : activeConfig->shapes)
					{
						Transform xform;
						xform.position = shape.position;
						xform.scale = shape.scale;
						xform.rotQuat = getRotQuatFromDegrees(shape.rotationDegrees);
						mat4 shapeModelMatrix = rootModelMat * xform.getModelMatrix();
						
						vec3 color = shapeIdx == selectedShapeIdx ? vec3(1.f, 1.f, 0.25f) : vec3(1, 0, 0);
						collisionShapeShader->use();
						collisionShapeShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
						collisionShapeShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
						collisionShapeShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(shapeModelMatrix));
						collisionShapeShader->setUniform3f("color", color);

						SpaceArcade& game = SpaceArcade::get();
						static auto renderShape = [](ECollisionShape shape, const sp<Shader>& shader) {
							const sp<const SAT::Model>& modelForShape = SpaceArcade::get().getCollisionShapeFactoryRef().getModelForShape(shape);
							modelForShape->draw(*shader);
						};

						ECollisionShape shapeType = static_cast<ECollisionShape>(shape.shape);
						switch (shapeType)
						{
							case ECollisionShape::CUBE:
								{
									//doesn't actually use collision shader above, but other collision shapes will
									shapeRenderer->renderUnitCube({ shapeModelMatrix, view, projection, color, renderMode, /*will correct after loop*/renderMode });
									break;
								}
							case ECollisionShape::POLYCAPSULE:
								{
									capsuleRenderer->render();
									break;
								}
							case ECollisionShape::WEDGE:
								{
									renderShape(ECollisionShape::WEDGE, collisionShapeShader);
									break;
								}
							case ECollisionShape::PYRAMID:
								{
									renderShape(ECollisionShape::PYRAMID, collisionShapeShader);
									break;
								}
							case ECollisionShape::UVSPHERE:
								{
									renderShape(ECollisionShape::UVSPHERE, collisionShapeShader);
									break;
								}
							case ECollisionShape::ICOSPHERE:
								{
									renderShape(ECollisionShape::ICOSPHERE, collisionShapeShader);
									break;
								}
							case ECollisionShape::MODEL:
								{
									//this will spam logs when model can't be found. :( but not wanting to over engineer this system right now.
									if (shape.modelFilePath.length() != 0)
									{
										auto findResult = collisionModels.find(shape.modelFilePath);
										if (findResult == collisionModels.end())
										{
											tryLoadCollisionModel(shape.modelFilePath.c_str());
										}

										findResult = collisionModels.find(shape.modelFilePath);
										if(findResult != collisionModels.end())
										{
											auto findSR = collisionModelSATShapesRenders.find(shape.modelFilePath);
											if (findSR != collisionModelSATShapesRenders.end())
											{
												Transform xform;
												xform.position = shape.position;
												xform.scale = shape.scale;
												xform.rotQuat = getRotQuatFromDegrees(shape.rotationDegrees);
												findSR->second->setXform(xform);

												ShapeRenderWrapper::RenderOverrides overrides;
												overrides.shader = collisionShapeShader.get();
												overrides.parentXform = &rootModelMat;
												findSR->second->render(overrides); //use collision model shader to get selection color features
											}
										}
									}
									break;
								}
							default:
								{
									//show a cube if something went wrong
									shapeRenderer->renderUnitCube({ shapeModelMatrix, view, projection, color, renderMode, /*will correct after loop*/renderMode });
									break;
								}
						}

						++shapeIdx;
					}
					ec(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

				}
				if (bool bRenderSpawnPonts = true)
				{
					for (int spawnPointIdx = 0; spawnPointIdx < int(activeConfig->spawnPoints.size()); ++spawnPointIdx)
					{
						FighterSpawnPoint& spawn = activeConfig->spawnPoints[spawnPointIdx];

						vec4 spawnPnt_wp = rootModelMat * vec4(spawn.location_lp, 1.f);
						mat4 pnt_m{ 1.f }; //make separate matrix so scaling doesn't affect visuals
						pnt_m = glm::translate(pnt_m, vec3(spawnPnt_wp));
						pnt_m = glm::scale(pnt_m, vec3(0.1f));

						vec4 spawnDir_wn = normalize(rootModelMat * vec4(spawn.direction_ln, 0.f));

						DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
						debugRenderSystem.renderCube(pnt_m,			spawnPointIdx == selectedSpawnPointIdx ? color::metalicgold() : color::lightOrange());
						debugRenderSystem.renderRay(spawnDir_wn, spawnPnt_wp, spawnPointIdx == selectedSpawnPointIdx ? color::metalicgold() : color::lightOrange());
					}
				}
			}
		}
	}


}

