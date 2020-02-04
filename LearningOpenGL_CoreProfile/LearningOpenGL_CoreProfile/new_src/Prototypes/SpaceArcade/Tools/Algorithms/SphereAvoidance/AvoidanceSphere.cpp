#include "AvoidanceSphere.h"
#include "../../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../../../GameFramework/SADebugRenderSystem.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SALevelSystem.h"
#include "../../../GameFramework/SALevel.h"

namespace SA
{
	using namespace glm;

	AvoidanceSphere::AvoidanceSphere(float radius, const glm::vec3& position) : 
		radius(radius)
	{
		localXform.position = position;
		applyXform();
	}

	void AvoidanceSphere::postConstruct()
	{
		static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &AvoidanceSphere::handlePreLevelChange);
		levelSystem.onPostLevelChange.addWeakObj(sp_this(), &AvoidanceSphere::handlePostLevelChange);
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			handlePostLevelChange(nullptr, currentLevel);
		}
	}

	void AvoidanceSphere::render() const
	{
		glm::mat4 model = getWorldMat();

		DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
		debugRenderSystem.renderSphere(model, debugColor);
	}

	void AvoidanceSphere::setPosition(glm::vec3 position)
	{
		//#TODO #scenenodes #partial_transform would be better to have this as a scene node that accepts a partial transform that doesn't include scale.
		//or setting position could just be setting local position
		localXform.position = position;
		applyXform();
	}

	void AvoidanceSphere::setRadius(float radius)
	{
		this->radius = radius;
		applyXform();
	}

	void AvoidanceSphere::setParentXform(const glm::mat4& newParentXform)
	{
		parentXform = newParentXform;
		applyXform();
	}

	void AvoidanceSphere::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		//dtor of hash entry will do cleanup
		myGridEntry = nullptr;
		cachedAvoidanceGrid = nullptr;
	}

	void AvoidanceSphere::handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (SH::SpatialHashGrid<AvoidanceSphere>* avoidanceGrid = newCurrentLevel->getTypedGrid<AvoidanceSphere>())
		{
			if (!myGridEntry)
			{
				myGridEntry = avoidanceGrid->insert(*this, getOBB());
				cachedAvoidanceGrid = avoidanceGrid;
			}
			else
			{
				//it is possible for the avoidance sphere to be constructed during level start up, which means it will already set up the spatial hash.
				//in that case, this handler will be called after the initial avoidance sphere is put in the hash, meaning we can't just assert that myGridEntry is empty.
				//so, if we do get to this point, make sure that our entry is for the current avoidance grid.
				assert(cachedAvoidanceGrid == avoidanceGrid);
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "failed to get the avoidance sphere grid");
		}
	}

	glm::mat4 AvoidanceSphere::getWorldMat() const
{
		return parentXform * localXform.getModelMatrix();
	}

	void AvoidanceSphere::applyXform()
	{
		static const float scaleUpFactor = 1.f / glm::length(vec3(SH::AABB[0])); //correct for cube verts not being length 1 from origin.
		float cubeScaleUp = radius * scaleUpFactor; //unit cube
		localXform.scale = glm::vec3(cubeScaleUp);

		glm::mat4 model_m = getWorldMat();

		for (size_t vert = 0; vert < SH::AABB.size(); ++vert)
		{
			AABB[vert] = model_m * SH::AABB[vert];
		}

		if (myGridEntry && cachedAvoidanceGrid)
		{
			cachedAvoidanceGrid->updateEntry(myGridEntry, getOBB());
		}
	}

}

