#include "AvoidanceSphere.h"
#include "../../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../../../GameFramework/SADebugRenderSystem.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SALevelSystem.h"
#include "../../../GameFramework/SALevel.h"
#include "../../SAUtilities.h"

namespace SA
{
	using namespace glm;

	AvoidanceSphere::AvoidanceSphere(float radius, const glm::vec3& position, const sp<GameEntity>& owner) 
	{
		setRadius(radius);
		owningEntity = owner;
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

		//take care not to apply scale of parent
		glm::mat4 model_m = glm::translate(glm::mat4(1.0f), cachedWorldPos);
		model_m = glm::scale(model_m, glm::vec3(radius));
		debugRenderSystem.renderSphere(model, debugColor);

		if constexpr (bCompileDebugDebugSpatialHashVisualizations)
		{
			if (myGridEntry && cachedAvoidanceGrid && bDebugSpatialHashVisualization)
			{
				static std::vector<sp<const SH::HashCell<AvoidanceSphere>>> outCells;
				cachedAvoidanceGrid->lookupCellsForEntry(*myGridEntry, outCells);

				vec3 gridCenterOffset = cachedAvoidanceGrid->gridCellSize / 2.f;
				for (auto cell : outCells)
				{
					glm::mat4 cellModel_m{ 1.f };
					cellModel_m = glm::translate(cellModel_m, glm::vec3(cell->location) * cachedAvoidanceGrid->gridCellSize + gridCenterOffset);
					cellModel_m = glm::scale(cellModel_m, cachedAvoidanceGrid->gridCellSize);
					debugRenderSystem.renderCube(cellModel_m, vec3(0, 0, 1));
				}
			}
		}
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
		this->radius2 = radius * radius;
		assert(!Utils::float_equals(radius, 0.f));
		assert(!Utils::float_equals(radius2, 0.f));
		applyXform();
	}

	void AvoidanceSphere::setParentXform(const glm::mat4& newParentXform)
	{
		parentXform = newParentXform;
		applyXform();
	}

	glm::vec3 AvoidanceSphere::getWorldPosition() const
	{
		//optmizing this with a cached value as it is a hot function that gets called frequently.
		return cachedWorldPos;
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

	glm::mat4 AvoidanceSphere::getOOBWorldMat() const
	{
		return parentXform * localOOBXform.getModelMatrix();
	}

	void AvoidanceSphere::applyXform()
	{
		constexpr float scaleUpFactor = 1.f / 0.5f; //unit cube has length of 0.5 from center to face
		float cubeScaleUp = radius * scaleUpFactor; //unit cube

		localOOBXform = localXform;
		localOOBXform.scale = glm::vec3(cubeScaleUp);

		localXform.scale = glm::vec3(radius);

		glm::mat4 cubeModel_m = getOOBWorldMat();

		for (size_t vert = 0; vert < SH::AABB.size(); ++vert)
		{
			AABB[vert] = cubeModel_m * SH::AABB[vert];
		}

		if (myGridEntry && cachedAvoidanceGrid)
		{
			cachedAvoidanceGrid->updateEntry(myGridEntry, getOBB());
		}

		cacheProperties(getWorldMat());
	}

	void AvoidanceSphere::cacheProperties(const glm::mat4& sphereModel_m)
	{
		cachedWorldPos = vec3(parentXform * vec4(localXform.position, 1.f));
	}

	bool AvoidanceSphere::bDebugSpatialHashVisualization = false;
}

