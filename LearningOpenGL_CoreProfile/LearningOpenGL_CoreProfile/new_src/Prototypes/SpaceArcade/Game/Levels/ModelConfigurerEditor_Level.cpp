#include "ModelConfigurerEditor_Level.h"

#include <filesystem>

#include "../../../../../Libraries/imgui.1.69.gl/imgui.h"

#include "../SpaceArcade.h"
#include "../SAUISystem.h"
#include "../SAModSystem.h"
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
		collisionShapeShader = new_sp<SA::Shader>(SAT::DebugShapeVertSrc, SAT::DebugShapeFragSrc, false);
		shapeRenderer = new_sp<PrimitiveShapeRenderer>();

		if (!polyShape)
		{
			polyShape = new_sp<SAT::PolygonCapsuleShape>();
			cubeShape = new_sp<SAT::CubeShape>();
		}
		capsuleRenderer = new_sp<SAT::CapsuleRenderer>();

		const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
		if (zeroPlayer)
		{
			if (const sp<CameraBase>& camera = zeroPlayer->getCamera())
			{
				//set ability to view large models
				camera->setFar(1000.f);
			}
		}
	}

	void ModelConfigurerEditor_Level::endLevel_v()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getUISystem()->onUIFrameStarted.removeStrong(sp_this(), &ModelConfigurerEditor_Level::handleUIFrameStarted);

		//cleaning up OpenGL resources immediately, these should get cleaned up with their dtors when this dtor is hit; so probably unecessary to do here
		model3DShader = nullptr;
		collisionShapeShader = nullptr;
		shapeRenderer = nullptr;
		capsuleRenderer = nullptr;
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
			if (ImGui::CollapsingHeader("VIEWPORT UI", ImGuiTreeNodeFlags_DefaultOpen))
			{
				renderUI_ViewportUI();
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
			camera->setPosition({ 5, 0, 0 });
			camera->lookAt_v((camera->getPosition() + glm::vec3{ -1, 0, 0 }));
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
			for (CollisionShapeConfig& shapeConfig : activeConfig->shapes)
			{
				snprintf(tempTextBuffer, sizeof(tempTextBuffer), "shape %d : %s", shapeIdx, shapeToStr((ECollisionShape) shapeConfig.shape));
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
						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Scale Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].scale.x);

						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Rotation Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].rotationDegrees.x);

						snprintf(tempTextBuffer, sizeof(tempTextBuffer), "Translation Shape %d", selectedShapeIdx);
						ImGui::InputFloat3(tempTextBuffer, &activeConfig->shapes[selectedShapeIdx].position.x);

						ImGui::RadioButton("Cube (3e 3f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::CUBE);
						ImGui::SameLine(); ImGui::RadioButton("PolyCapsule (7e 6f)", &activeConfig->shapes[selectedShapeIdx].shape, (int)ECollisionShape::POLYCAPSULE);
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

	void ModelConfigurerEditor_Level::renderUI_ViewportUI()
	{
		ImGui::Separator();
		if (ImGui::SliderFloat("Camera Speed", &cameraSpeedModifier, 1.0f, 10.f))
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
		ImGui::Separator();
	}

	void ModelConfigurerEditor_Level::renderUI_Team()
	{
		char strBuffer[256];
		constexpr size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);

		ImGui::Separator();
		if (activeConfig)
		{
			if (activeConfig->teamData.size() == 0)
			{
				activeConfig->teamData.push_back({});
			}

			for (size_t teamIdx = 0; teamIdx < activeConfig->teamData.size(); ++teamIdx)
			{
				TeamData& teamData = activeConfig->teamData[teamIdx];
				snprintf(strBuffer, strBufferSize, "Team %d", teamIdx);
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

			activeConfig = newConfig;
		}
	}

	void ModelConfigurerEditor_Level::onActiveConfigSet(const SpawnConfig& newConfig)
	{

		std::string modelPath = activeConfig->getModelFilePath();

		//may fail to load model if user provided bad path, either way we need to set to null or valid model
		renderModel = SpaceArcade::get().getAssetSystem().loadModel(modelPath.c_str());

		primaryProjectileConfig = activeConfig->getPrimaryProjectileConfig();

	}

	void ModelConfigurerEditor_Level::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		//#TODO refactor this to use the CollisionDebugRenderer class

		if (renderModel && shapeRenderer && activeConfig)
		{
			Transform rootXform;
			rootXform.position = activeConfig->modelPosition;
			rootXform.scale = activeConfig->modelScale;
			rootXform.rotQuat = getRotQuatFromDegrees(activeConfig->modelRotationDegrees);
			mat4 rootModelMat = rootXform.getModelMatrix();

			const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
			if (zeroPlayer)
			{
				const sp<CameraBase>& camera = zeroPlayer->getCamera();

				shapeRenderer->renderAxes(mat4(1.f), view, projection);
				
				{ //render model
					model3DShader->use();
					//model3DShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
					//model3DShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0));
					//model3DShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
					//model3DShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0));
					//model3DShader->setUniform1i("material.shininess", 32);
					model3DShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
					model3DShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
					model3DShader->setUniform3f("cameraPosition", camera->getPosition());
					model3DShader->setUniform3f("tint", activeTeamData.teamTint);
					model3DShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(rootModelMat));
					renderModel->draw(*model3DShader);
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

				if (bRenderCollisionShapes)
				{
					GLenum renderMode = bRenderCollisionShapesLines ? GL_LINE : GL_FILL;
					ec(glPolygonMode(GL_FRONT_AND_BACK, renderMode)); 

					int shapeIdx = 0;
					for (CollisionShapeConfig shape : activeConfig->shapes)
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
			}
		}
	}
}

