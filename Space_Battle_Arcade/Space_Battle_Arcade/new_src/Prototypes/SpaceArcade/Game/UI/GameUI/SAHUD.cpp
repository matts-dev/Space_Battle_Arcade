#include "SAHUD.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SAWindowSystem.h"
#include "../../../Rendering/BuiltInShaders.h"
#include "../../../Rendering/SAGPUResource.h"
#include "../../../Rendering/SAShader.h"
#include "../../../Tools/Geometry/SimpleShapes.h"
#include "../../../Rendering/Camera/Texture_2D.h"
#include "Widgets3D/Widget3D_Respawn.h"
#include "Widgets3D/Widget3D_DigitalClockFontTest.h"
#include "../../../Tools/DataStructures/SATransform.h"
#include "../../GameSystems/SAUISystem_Game.h"
#include "../../SpaceArcade.h"
#include "Widgets3D/HUD/Widget3D_PlayerStatusBarBase.h"
#include "../../../GameFramework/SADebugRenderSystem.h"
#include "../../../GameFramework/GameMode/ServerGameMode_Base.h"
#include "../../../GameFramework/SALevel.h"
#include "../../../GameFramework/SALevelSystem.h"
#include "../../../GameFramework/SAPlayerBase.h"
#include "../../../GameFramework/SAPlayerSystem.h"
#include "text/GlitchText.h"
#include "../../SAPlayer.h"
#include "Widgets3D/Widget3D_LaserButton.h"
#include "../../Levels/MainMenuLevel.h"
#include "../../../Tools/PlatformUtils.h"
#include "../../../GameFramework/Input/SAInput.h"

using namespace glm;

namespace SA
{
	void HUD::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &HUD::handleGameUIRender);

		reticleTexture = new_sp<Texture_2D>("GameData/engine_assets/SimpleCrosshair.png");
		quadShape = new_sp<TexturedQuad>();
		spriteShader = new_sp<Shader>(spriteVS_src, spriteFS_src, false);

		respawnWidget = new_sp<Widget3D_Respawn>();

		healthBar = new_sp<Widget3D_HealthBar>(playerIdx);
		energyBar = new_sp<Widget3D_EnergyBar>(playerIdx);

		//escapeMenuLeaveButton = new_sp<Widget3D_LaserButton>("Leave Level");
		//escapeMenuLeaveButton->onClickedDelegate.addWeakObj(sp_this(), &HUD::handleReturnToMainMenuClicked);
		//escapeMenuLeaveButton->activate(false);

		DigitalClockFont::Data textInit_LeaveButton;
		textInit_LeaveButton.text = "Leave Level: [SHIFT + ENTER]";
		escapeButtonText_Leave = new_sp<GlitchTextFont>(textInit_LeaveButton);
		escapeButtonText_Leave->resetAnim();
		escapeButtonText_Leave->setHorizontalPivot(DigitalClockFont::EHorizontalPivot::CENTER);
		escapeButtonText_Leave->setAnimPlayForward(bEscapeMenuOpen);
		escapeButtonText_Leave->play(true);
		escapeButtonText_Leave->completeAnimation();


		textInit_LeaveButton.text = "RESUME: [ESCAPE]";
		escapeButtonText_Resume = new_sp<GlitchTextFont>(textInit_LeaveButton);
		escapeButtonText_Resume->resetAnim();
		escapeButtonText_Resume->setHorizontalPivot(DigitalClockFont::EHorizontalPivot::CENTER);
		escapeButtonText_Resume->setAnimPlayForward(bEscapeMenuOpen);
		escapeButtonText_Resume->play(true);
		escapeButtonText_Resume->completeAnimation();

		DigitalClockFont::Data textInit_slowmo;
		textInit_slowmo.text = "slowmo ready";
		slowmoText = new_sp<GlitchTextFont>(textInit_slowmo);
		slowmoText->resetAnim();
		slowmoText->play(true);
		slowmoText->setAnimPlayForward(true);

		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(playerIdx))
		{
			player->getInput().getKeyEvent(GLFW_KEY_ENTER).addWeakObj(sp_this(), &HUD::handleEnterPressed);
			player->getInput().getKeyEvent(GLFW_KEY_KP_ENTER).addWeakObj(sp_this(), &HUD::handleEnterPressed);
			
		} else { STOP_DEBUGGER_HERE() } //we didn't get player so main menu enter+shift will not return player to main menu!

#if HUD_FONT_TEST 
		fontTest = new_sp<class Widget3D_DigitalClockFontTest>();
#endif
	}

	void HUD::tryRegenerateTeamWidgets()
	{
		//alternatively we could hook into level changed events and update, but there potentially is some data races. 
		//so for now doing a check in a tick to hopefully get this project complete.
		size_t numTeams = 2; 
		
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (ServerGameMode_Base* GMBase = currentLevel->getGameModeBase())
			{
				numTeams = GMBase->getNumberOfCurrentTeams();
			}
		}

		if (numTeams != teamHealthBars.size())
		{
			const sp<PlayerBase>& player = SpaceArcade::get().getPlayerSystem().getPlayer(playerIdx);

			//grow array
			for(size_t teamIdx = teamHealthBars.size(); teamIdx < numTeams; ++teamIdx)
			{
				teamHealthBars.push_back(new_sp<Widget3D_TeamProgressBar>(playerIdx, teamIdx));

				if (player && player->hasControlTarget())
				{
					teamHealthBars.back()->activate(true); //fix bug where for first life, in main menu you would not see team health bars
				}
				
			}

			//shrink array
			if (numTeams < teamHealthBars.size())
			{
				teamHealthBars.resize(numTeams); 
			}
		}
	}

	void HUD::handleReturnToMainMenuClicked()
	{
		if (starJumpTimer != nullptr)
		{
			return; //we're already leaving the level
		}

		setEscapeMenuOpen(false); //don't have this open if it ends up being shared when we go back to the main menu, close it now.

		const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
		const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
		SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get());
		if (spaceLevel && worldTimeManager)
		{
			starJumpTimer = new_sp<MultiDelegate<>>();
			starJumpTimer->addWeakObj(sp_this(), &HUD::handleStarJumpOver);

			worldTimeManager->createTimer(starJumpTimer, EndGameParameters{}.delayTransitionMainmenuSec); //#TODO make this a flag somewhere instead of creating a struct
			spaceLevel->enableStarJump(true);
		}
		else
		{
			SpaceLevelBase::transitionToMainMenu_s();
		}
	}

	void HUD::handleStarJumpOver()
	{
		starJumpTimer = nullptr;
		SpaceLevelBase::transitionToMainMenu_s();
	}

	void HUD::handleEnterPressed(int state, int modifier_keys, int scancode) //#TODO #nextengine have enums that are equal to GLFW states so I don't haev to guess if I am comparing correct things without looking at a code reference.
	{
		if (state == GLFW_PRESS 
			&& modifier_keys == GLFW_MOD_SHIFT
			&& bEscapeMenuOpen)
		{
			handleReturnToMainMenuClicked(); //can't do a button for now, but so just leaving button click delegate in place and calling it directly.
			setEscapeMenuOpen(false);
			escapeButtonText_Leave->completeAnimation(); //should be animating away due to settings escape menu closed call
			escapeButtonText_Resume->completeAnimation(); //should be animating away due to settings escape menu closed call
		}
	}

	void HUD::setEscapeMenuOpen(bool bNewValue)
	{
		if (bNewValue != bEscapeMenuOpen)
		{
			bEscapeMenuOpen = bNewValue;

			//only bother activating escape menu buttons (eg leave button) if we're not in the main menu
			if (bool NotInMainMenu = dynamic_cast<MainMenuLevel*>(GameBase::get().getLevelSystem().getCurrentLevel().get()) == nullptr)
			{
				//escapeMenuLeaveButton->activate(bEscapeMenuOpen);
				escapeButtonText_Leave->setAnimPlayForward(bEscapeMenuOpen);
				escapeButtonText_Resume->setAnimPlayForward(bEscapeMenuOpen);
			}
		}
	}

	void HUD::handleGameUIRender(GameUIRenderData& rd_ui)
	{
		const RenderData* rd_game = rd_ui.renderData();
		if (bRenderHUD && rd_game)
		{
			tryRegenerateTeamWidgets();

			bool bPlayerSlomoReady = false;
			const sp<PlayerBase>& playerBase = GameBase::get().getPlayerSystem().getPlayer(playerIdx);
			if (playerBase && slowmoText)
			{
				if (playerBase->hasControlTarget())
				{
					//sucks to dynamic cast here every frame, but I don't want to make a separate component just for this; we could static cast but that is dangerous territory as it creates a refactoring hidden bomb 
					if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerBase.get()))
					{
						bPlayerSlomoReady = player->canDilateTime();
					}
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// this is a bit of a hack. originally I anticipated a distance of 10.f in front of hud was a good amount
			// but it isn't great because HUD widgets will clip into 3d structures. There's obvious two ways to fix 
			// this, one lower the distance, or 2 always draw ui without consideration of depth. both are costly in that the
			// laser system is largly independent and desigend to be drawn with 3D depth and that the main menu has scaled 
			// elements based on a distance of 10.f. A simple fix is to just recalculate HUD data with a shorter distance only
			// for the in game hud. So we copy the hud data and recalcualte it at a new distnace
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			HUDData3D hudData = rd_ui.getHUDData3D(); //make a copy
			if (CameraBase* gameCam = rd_ui.camera())
			{
				hudData.frontOffsetDist = 1.f; //be careful, this must be beyond near clip plane!
				calculateHUDData3D(hudData, *gameCam, rd_ui);
			}
			float distScaleCorrection = hudData.frontOffsetDist / 10.f; //10.f is the default distance for hudData

			////////////////////////////////////////////////////////
			// draw reticle
			////////////////////////////////////////////////////////
			if (spriteShader && !bEscapeMenuOpen)
			{
				//draw reticle
				if (quadShape && reticleTexture && reticleTexture->hasAcquiredResources() && spriteShader)
				{
					float reticalSize = 0.015f * float(rd_ui.frameBuffer_MinDimension()); //X% of smallest screen dimension 

					mat4 model(1.f);
					model = glm::scale(model, vec3(reticalSize));

					spriteShader->use();
					spriteShader->setUniform1i("textureData", 0);
					spriteShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
					spriteShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(rd_ui.orthographicProjection_m()));
					reticleTexture->bindTexture(GL_TEXTURE0);
					quadShape->render();
				}
			}

			////////////////////////////////////////////////////////
			// calculations
			////////////////////////////////////////////////////////
#define RENDER_PLAYER_HUD_ELEMENTS 1
#if RENDER_PLAYER_HUD_ELEMENTS
			const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(playerIdx);
			IControllable* controlTarget = player ? player->getControlTarget() : nullptr;
			bool bHasControlTarget = controlTarget != nullptr;

			vec3 hudCenterPoint = hudData.camPos + (hudData.frontOffsetDist * hudData.camFront);
			vec2 statusBarOffsetPerc{ 0.75f, -0.8f};
			//vec3 healthBarLoc = hudCenterPoint;
			//vec3 energyBarLoc = hudCenterPoint;
			//vec3 statusBarTextOffset = vec3(0.f);
			vec3 healthBarLoc = hudCenterPoint + hudData.camRight * -(statusBarOffsetPerc.x*hudData.savezoneMax_x) + hudData.camUp * (statusBarOffsetPerc.y *hudData.savezoneMax_y);
			vec3 energyBarLoc = hudCenterPoint + hudData.camRight * (statusBarOffsetPerc.x*hudData.savezoneMax_x) + hudData.camUp * (statusBarOffsetPerc.y *hudData.savezoneMax_y);
			vec3 statusBarTextOffset = (hudData.camUp * hudData.savezoneMax_y * /*perc*/0.1f);
			
			//calculated out of control branch so debugging code can read these transforms
			Transform healthBarXform;
			healthBarXform.rotQuat = rd_ui.camQuat();
			healthBarXform.position = healthBarLoc;
			healthBarXform.scale = vec3(2.f, 1.f, 1.f) * distScaleCorrection;
			healthBar->setBarTransform(healthBarXform);
			Transform healthBarTextXform = healthBarXform;
			healthBarTextXform.scale = vec3(hudData.textScale);
			healthBarTextXform.position += statusBarTextOffset;
			healthBar->setTextTransform(healthBarTextXform);
			healthBar->renderGameUI(rd_ui); //must come after transform update

			Transform energyBarXform = healthBarXform; //copy quat and scale
			energyBarXform.position = energyBarLoc;
			energyBar->setBarTransform(energyBarXform);
			Transform energyBarTextXform = energyBarXform;
			energyBarTextXform.scale = vec3(hudData.textScale);
			energyBarTextXform.position += statusBarTextOffset;
			energyBar->setTextTransform(energyBarTextXform);
			energyBar->renderGameUI(rd_ui); //must come after transform update

			Transform slowmoTextXform = energyBarXform; //copy previous xform and just change what we need
			slowmoTextXform.scale = distScaleCorrection * vec3(0.075f);
			slowmoTextXform.position = hudCenterPoint + hudData.camUp * ((0.1f + -statusBarOffsetPerc.y) *hudData.savezoneMax_y);
			slowmoText->setXform(slowmoTextXform);
			slowmoText->tick(rd_ui.dt_sec());
			if (bPlayerSlomoReady)
			{
				slowmoText->render(*rd_game);
			}

			////////////////////////////////////////////////////////
			// respawn widget
			////////////////////////////////////////////////////////
			respawnWidget->renderGameUI(rd_ui);

			////////////////////////////////////////////////////////
			// escape menu
			////////////////////////////////////////////////////////
			Transform escapeMenuLeaveButtonXform;
			escapeMenuLeaveButtonXform.position = hudCenterPoint + hudData.camUp * (0.25f*hudData.savezoneMax_y);
			escapeMenuLeaveButtonXform.scale = vec3(0.01f);
			escapeMenuLeaveButtonXform.rotQuat = rd_ui.camQuat();
			//escapeMenuLeaveButton->setXform(escapeMenuLeaveButtonXform);
			escapeButtonText_Leave->setXform(escapeMenuLeaveButtonXform);
			escapeButtonText_Leave->tick(distScaleCorrection);
			escapeButtonText_Leave->render(*rd_game);

			escapeMenuLeaveButtonXform.position = hudCenterPoint + hudData.camUp * (0.35f*hudData.savezoneMax_y);
			escapeButtonText_Resume->setXform(escapeMenuLeaveButtonXform);
			escapeButtonText_Resume->tick(distScaleCorrection);
			escapeButtonText_Resume->render(*rd_game);

			////////////////////////////////////////////////////////
			// configure team bars
			////////////////////////////////////////////////////////
			if (teamHealthBars.size() > 0 && (bHasControlTarget || respawnWidget->isRespawning()))
			{
				float teamTopPerc = 0.25f; //we're spacing them out within the top X% of the screen.
				float percOffset = 1.f / float(teamHealthBars.size() - 1); //-1 because this works out for spacing for count of 2

				vec3 layoutDirection_v = hudData.camRight * (2*hudData.savezoneMax_x) * teamTopPerc;
				vec3 layoutStart_p = hudCenterPoint + (hudData.camUp * hudData.savezoneMax_y * 0.95f) - 0.5f*layoutDirection_v;
				for (size_t teamIdx = 0; teamIdx < teamHealthBars.size(); ++teamIdx)
				{
					//bar xform
					Transform xform;
					xform.position = layoutStart_p + layoutDirection_v * (float(teamIdx) * percOffset);
					float barScaleFactor = 0.25f;
					xform.scale = vec3(barScaleFactor / float(teamHealthBars.size()), 0.1f, 1.f);
					xform.rotQuat = rd_ui.camQuat();
					teamHealthBars[teamIdx]->setBarTransform(xform);

					//text xform
					xform.position += hudData.camUp * hudData.savezoneMax_y * -0.05f;
					xform.scale = vec3(hudData.textScale * 0.75f);
					teamHealthBars[teamIdx]->setTextTransform(xform);

					teamHealthBars[teamIdx]->renderGameUI(rd_ui);
				}
			}

#endif //RENDER_PLAYER_HUD_ELEMENTS

#if HUD_FONT_TEST 
			fontTest->renderGameUI(rd_ui);
#endif
			if constexpr (constexpr bool bDebugHudPlacement = false)
			{
				DebugRenderSystem& db = GameBase::get().getDebugRenderSystem();
				vec3 camNudge = vec3(0, 0.001f, 0);
				if (bool bNudgeCamForward = false) { camNudge += hudData.camFront * 10.f; }

				////////////////////////////////////////////////////////
				// render camera data
				////////////////////////////////////////////////////////
				float normScaleUp = 3.f;
				if (bool bDebugCamera = false) 
				{ 
					db.renderSphere(hudData.camPos, vec3(0.05f), vec3(0, 0, 1)); 
					db.renderLine(hudData.camPos + camNudge, hudData.camPos + camNudge + hudData.camRight*normScaleUp, vec3(1, 0, 0));
					db.renderLine(hudData.camPos + camNudge, hudData.camPos + camNudge + hudData.camUp*normScaleUp, vec3(0, 1, 0));
					db.renderLine(hudData.camPos + camNudge, hudData.camPos + camNudge + hudData.camFront*normScaleUp, vec3(0, 0, 1));
				}

				////////////////////////////////////////////////////////
				// render placement data
				////////////////////////////////////////////////////////
#if RENDER_PLAYER_HUD_ELEMENTS
				db.renderSphere(healthBarXform.position, vec3(0.005f), vec3(1, 0, 0));
				db.renderLine(hudData.camPos + camNudge, healthBarXform.position, vec3(1, 0, 0));
				db.renderLine(hudData.camPos + camNudge, energyBarXform.position, vec3(0, 1, 0));
#endif
			}
		}
	}


}

