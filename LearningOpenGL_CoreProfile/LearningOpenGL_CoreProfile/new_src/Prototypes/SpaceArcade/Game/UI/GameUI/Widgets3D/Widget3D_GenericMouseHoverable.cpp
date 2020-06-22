#include "Widget3D_GenericMouseHoverable.h"

#include <gtx/common.inl>
#include "../LaserUIPool.h"
#include "../../../SpaceArcade.h"
#include "../../../../GameFramework/SALevelSystem.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../../GameFramework/SATimeManagementSystem.h"
#include "../../../../Tools/DataStructures/SATransform.h"
#include "../../../../Tools/PlatformUtils.h"
#include "../../../../GameFramework/SADebugRenderSystem.h"

#define DEBUG_GENERIC_HOVERABLE 1

namespace SA
{
	using namespace glm;

	SA::Widget3D_GenericMouseHoverable::~Widget3D_GenericMouseHoverable()
	{
		releaseLasers();
	}

	Widget3D_GenericMouseHoverable::Widget3D_GenericMouseHoverable()
	{
		localAABB =
		{
			glm::vec4(-0.5f, -0.5f, -0.5f,	1.0f),
			glm::vec4(-0.5f, -0.5f, 0.5f,	1.0f),
			glm::vec4(-0.5f, 0.5f, -0.5f,	1.0f),
			glm::vec4(-0.5f, 0.5f, 0.5f,	1.0f),
			glm::vec4(0.5f, -0.5f, -0.5f,	1.0f),
			glm::vec4(0.5f, -0.5f, 0.5f,	1.0f),
			glm::vec4(0.5f, 0.5f, -0.5f,	1.0f),
			glm::vec4(0.5f, 0.5f, 0.5f,     1.0f),
		};
	}


	void SA::Widget3D_GenericMouseHoverable::postConstruct()
	{
		setRenderWithGameUIDispatch(true);
		Parent::postConstruct();

		if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem()) //#TODO should never be null... this needs a refactor to return reference
		{
			hashEntry = gameUISystem->getSpatialHash().insert(*this, getOBB());
		}
		

		////////////////////////////////////////////////////////
		// hook up to tick
		////////////////////////////////////////////////////////
		//LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		//levelSystem.onPreLevelChange.addWeakObj(sp_this(), &Widget3D_GenericMouseHoverable::handlePreLevelChange);
		//if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		//{
		//	handlePreLevelChange(nullptr, currentLevel);
		//}


	}

	void Widget3D_GenericMouseHoverable::setXform(const Transform& inXform)
	{
		xform = inXform;
		updateCollisionData();

		//if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem()) //#TODO should never be null... this needs a refactor to return reference
		//{
		//	hashEntry = gameUISystem->getSpatialHash().insert(*this, getOBB());
		//}
	}

	const std::array<glm::vec4, 8>& Widget3D_GenericMouseHoverable::getLocalAABB() const
	{
		return localAABB;
	}

	//void Widget3D_GenericMouseHoverable::onActivated()
	//{
	//	setRenderWithGameUIDispatch(true);
	//}

	//void Widget3D_GenericMouseHoverable::onDeactivated()
	//{
	//	setRenderWithGameUIDispatch(false);
	//}

	void Widget3D_GenericMouseHoverable::updateCollisionData()
	{
		if (hashEntry)
		{
			SpaceArcade::get().getGameUISystem()->getSpatialHash().updateEntry(hashEntry, getOBB());
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "No hash entry");
			STOP_DEBUGGER_HERE();
		}
	}

	//void SA::Widget3D_GenericMouseHoverable::tick_postCamera(float dt_sec)
	//{
	//}

	std::array<glm::vec4, 8> Widget3D_GenericMouseHoverable::getOBB() const
	{
		glm::mat4 xformMat = xform.getModelMatrix();
		std::array<glm::vec4, 8> OBB =
		{
			xformMat * localAABB[0],
			xformMat * localAABB[1],
			xformMat * localAABB[2],
			xformMat * localAABB[3],
			xformMat * localAABB[4],
			xformMat * localAABB[5],
			xformMat * localAABB[6],
			xformMat * localAABB[7]
		};
		return OBB;
	}

	glm::mat4 SA::Widget3D_GenericMouseHoverable::getModelMatrix() const
	{
		return xform.getModelMatrix();
	}

	glm::vec3 SA::Widget3D_GenericMouseHoverable::getWorldLocation() const
	{
		return xform.position;
	}

	void SA::Widget3D_GenericMouseHoverable::onHoveredhisTick()
	{
		bHoveredThisTick = true;
	}

	void SA::Widget3D_GenericMouseHoverable::onClickedThisTick()
	{
		onClicked.broadcast();

		//apply toggle after click broadcast so that widgets can clean up other toggle states (including this one) before this handles toggling itself
		if (bClickToggles)
		{
			bToggled = !bToggled;
		}
	}

	void SA::Widget3D_GenericMouseHoverable::renderGameUI(GameUIRenderData& ui_rd)
	{
		
		if ((bHoveredThisTick || bToggled) && isActive())
		{
			bHoveredThisTick = false; //consume this hover, it will be reset if still hovered on next frame

			if (lasers.size() == 0)
			{
				claimLasers();
			}

			float dt_sec = ui_rd.dt_sec();

			rotation_rad += rotationSpeedSec_rad * dt_sec;
			rotation_rad = glm::fmod<float>(rotation_rad, 2.f*glm::pi<float>());
		
			const vec3 cFront_n = ui_rd.getHUDData3D().camFront;
			const vec3 cRight_n = ui_rd.getHUDData3D().camRight;
			vec3 radiusVec_v = cRight_n * radius;
			vec3 rotVec_v = radiusVec_v;

			glm::quat q = glm::angleAxis(rotation_rad, cFront_n);

			float laserOffset_rad = (2*glm::pi<float>()) / float(lasers.size());
			glm::quat updateQ = glm::angleAxis(laserOffset_rad, cFront_n) ;

			for (const sp<LaserUIObject>& laser: lasers)
			{
				rotVec_v = q * radiusVec_v;
				laser->updateStart_NoAnimReset(xform.position + rotVec_v);
				q = updateQ * q;

				rotVec_v = q * radiusVec_v;
				laser->updateEnd_NoAnimReset(xform.position + rotVec_v);
			}
		}
		else
		{
			if (lasers.size() > 0)
			{
				releaseLasers();
			}
		}

#if DEBUG_GENERIC_CLICKABLE
		DebugRenderSystem& db = GameBase::get().getDebugRenderSystem();
		db.renderCube(xform.getModelMatrix(), glm::vec3(0.1f, 0.5f, 0.7f));
#endif

	}

	void SA::Widget3D_GenericMouseHoverable::releaseLasers()
	{
		if (lasers.size() > 0)
		{
			LaserUIPool& laserPool = LaserUIPool::get();
			for (sp<LaserUIObject>& laser : lasers)
			{
				laserPool.releaseLaser(laser);
			}

			//get rid of the nullptrs (array is now full of nullptrs)
			lasers.clear();
		}
	}

	void SA::Widget3D_GenericMouseHoverable::claimLasers()
	{
		if (lasers.size() == 0)
		{
			LaserUIPool& laserPool = LaserUIPool::get();
			for (size_t laserIdx = 0; laserIdx < numLasers; laserIdx++)
			{
				sp<LaserUIObject> laser = laserPool.requestLaserObject();

				laser->setAnimDurations(laserSpeedSecs, laserSpeedSecs);


				//give laser a arbitrary location for now, the tick 
				laser->animateStartTo(vec3(0.f));
				laser->animateEndTo(vec3(0.f));
				lasers.push_back(laser);
			}
		}
		else
		{
			/*attempting to claim more lasers after initial lasers have been claimed, this is certainly a bug unless things have changed!*/
			STOP_DEBUGGER_HERE();
		}
	}

	//void SA::Widget3D_GenericMouseHoverable::handlePreLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	//{
	//	having render dispatch do spin calculations as that will be more efficient
	//	if (previousLevel)
	//	{
	//		previousLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).removeWeak(sp_this(), &Widget3D_Slider::tick_postCamera);
	//	}

	//	if (newCurrentLevel)
	//	{
	//		newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().POST_CAMERA).addWeakObj(sp_this(), &Widget3D_Slider::tick_postCamera);
	//	}
	//}


}

