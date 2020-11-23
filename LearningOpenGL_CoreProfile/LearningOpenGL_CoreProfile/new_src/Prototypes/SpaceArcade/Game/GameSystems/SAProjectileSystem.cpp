#include "SAProjectileSystem.h"

#include "../../Tools/SAUtilities.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SACollisionUtils.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SATimeManagementSystem.h"

#include "../../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../AssetConfigs/SAProjectileConfig.h"
#include "../../GameFramework/Components/CollisionComponent.h"
#include "../../GameFramework/SAWorldEntity.h"
#include "../../GameFramework/SAAudioSystem.h"
#include "../AssetConfigs/SASpawnConfig.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Rendering/RenderData.h"
#include "../../Rendering/DeferredRendering/DeferredRendererStateMachine.h"
#include "../../Tools/PlatformUtils.h"
#include "../../Rendering/DeferredRendering/DeferredRenderingShaders.h"

namespace SA
{
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Actual Projectile Instances; these system is responsible for creating these instances
	///////////////////////////////////////////////////////////////////////////////////////////////
	void Projectile::tick(float dt_sec, LevelBase& currentLevel)
	{
		using glm::mat4; using glm::vec3; using glm::quat; using glm::vec4;

		//#optimize perhaps cache level system in local static for perf; profiler may cacn help to determine if this is worth it
		timeAlive += dt_sec;

		float dt_distance = dt_sec * speed;

		stretchToDistance(dt_distance, true, currentLevel);
	}

	void Projectile::stretchToDistance(float dt_distance, float bDoCollisionTest, LevelBase& currentLevel)
	{
		using glm::mat4; using glm::vec3; using glm::quat; using glm::vec4;

		//if the projectile hit its target, change logic so it just stretches to hit location
		if (bHit)
		{
			forceRelease = true;
			return;
		}

		//#optimize investigate whether some of the matrices below can be cached once (eg fire rotation? offsetDirection?)
		vec3 start = xform.position;

		if(bCorrectPosition)
		{
			vec3 originalOffset_v = offsetStartPos - traceStartPos;
			vec3 toCurPos_v = xform.position - traceStartPos; //hypotenuse 
			vec3 projOntoCenteredLine_v = Utils::project(toCurPos_v, direction_n); //ie the line as if we fired from center of ship, 


			//consider the triangle made from the projection, and the vector to current position from trace start. the "opposite" line is same as in SOA 
			vec3 triOpposite_v = toCurPos_v - projOntoCenteredLine_v;
			float blendAlpha = glm::clamp(1.f - glm::length(projOntoCenteredLine_v) / offsetTraceCorrectionDistance, 0.f, 1.f); //[0,1]
			vec3 correctedPosition = (blendAlpha * originalOffset_v) + projOntoCenteredLine_v + traceStartPos;

			//slide this position into place so that it gradually aligns with the trace from center of spawn
			start = correctedPosition;
		}

		vec3 end = start + dt_distance * direction_n;
		vec3 offsetDir = glm::normalize(start - end);
		float offsetLength = dt_distance / 2;
		vec3 zOffset = vec3(0, 0, offsetLength);

		vec3 modelScaleStrech(1.f);
		modelScaleStrech.z = dt_distance / aabbSize.z;

		vec3 collisionBoxScaleStretch = aabbSize;
		collisionBoxScaleStretch.z = dt_distance;

		mat4 transToEnd_rotToFireDir_zOffset = glm::translate(glm::mat4(1.f), end);
		transToEnd_rotToFireDir_zOffset = transToEnd_rotToFireDir_zOffset * glm::toMat4(directionQuat); //#TODO this could also be xform.rot... which to use?
		transToEnd_rotToFireDir_zOffset = glm::translate(transToEnd_rotToFireDir_zOffset, zOffset);

		//model matrix composition: translateToEnd * rotateToFireDirection * OffsetZValueSoTipAtPoint * StretchToFitDistance
		collisionXform = glm::scale(transToEnd_rotToFireDir_zOffset, collisionBoxScaleStretch);
		renderXform = glm::scale(transToEnd_rotToFireDir_zOffset, modelScaleStrech);

		// models parallel to z
		xform.position = end;
#if _WIN32 && _DEBUG
		if (Utils::anyValueNAN(start)){__debugbreak();}
		if (Utils::anyValueNAN(end)) {__debugbreak();}
#endif //_WIN32

		if (soundEmitter)
		{
			soundEmitter->setPosition(end);
			soundEmitter->setVelocity(speed * direction_n);
		}

		if (pointLight)
		{
			pointLight->setPosition(end);
		}

		//collision check
		if (bDoCollisionTest && !bHit)
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = currentLevel.getWorldGrid();

			/* #optimize #alternative collision. perhaps simple ray casts would be faster; it will require that triangles for collision shapes are available
				This method isn't going to be ideal for large structures, because the distance to the center of the shape collided with isn't going
				to produce a distance to generate an accurate end point; a ray trace will however. Plus it will probably be more efficient than
				a cube x cube collision test.
			*/
			std::vector<std::shared_ptr<const SH::HashCell<WorldEntity>>> cells;
			worldGrid.lookupCellsForLine(start, end, cells);

			//#optimize below may be slow; but don't want a bunch of collisions with same objects that occupy large # of cells (eg large ship)
			static std::set<WorldEntity*> potentialCollisions;
			potentialCollisions.clear(); //absolute must do this since it is a static local

			for (sp<const SH::HashCell<WorldEntity>> cell : cells)
			{
				for (std::shared_ptr<SH::GridNode<WorldEntity>> gridNode : cell->nodeBucket)
				{
					potentialCollisions.insert(&gridNode->element);
				}
			}

			static sp<SAT::Shape> projectileShape = new_sp<SAT::CubeShape>();
			projectileShape->updateTransform(collisionXform);

			float smallestDistanceCollision_2 = std::numeric_limits<float>::infinity();
			WorldEntity* collidingEntity = nullptr;

			for (WorldEntity* entity : potentialCollisions)
			{
				CollisionComponent* collisionComp = entity->getGameComponent<CollisionComponent>();
				if (collisionComp && entity != owner.get())
				{
					const CollisionData* colisionData = collisionComp->getCollisionData();
					const sp<const SAT::Shape>& OBBShape = colisionData->getOBBShape();

					glm::vec4 obbMTV;
					if (SAT::Shape::CollisionTest(*projectileShape, *OBBShape, obbMTV))
					{
						//#TODO perhaps this shouldn't find the closest shape it collided with? will be redundant
						for (const CollisionData::ConstShapeData& shapeData : colisionData->getConstShapeData())
						{
							glm::vec4 mtv;
							if (SAT::Shape::CollisionTest(*projectileShape, *shapeData.shape, mtv))
							{
								vec3 shapeOrigin = vec3(shapeData.shape->getTransformedOrigin());

								//it would be better to use distance the contact point, but that isn't available in my SAT implementation
								//perhaps I will change that if I figure out an efficient means to get the contact point
								float distToShapOri = glm::length2(shapeOrigin - start); //get distance from start to shape ori
								if (distToShapOri < smallestDistanceCollision_2)
								{
									smallestDistanceCollision_2 = distToShapOri;
									collidingEntity = entity;
								}
							}
						}
					}
				}
			}

			if (collidingEntity)
			{
				float hitDistance = glm::sqrt(smallestDistanceCollision_2);
				glm::vec3 hit = start + (-offsetDir * hitDistance / 2.f); //div by 2 may not be right theoretically, but hit distance appears too far and this seems to fix

				//#TODO #componentize this interface to be a component to avoid dynamic cast. This will require comp delegate notify owner though, which may be slower than dyn cast; profiling and optimizatin likely needed.
				if (IProjectileHitNotifiable* toNotify = dynamic_cast<IProjectileHitNotifiable*>(collidingEntity))
				{
					toNotify->notifyProjectileCollision(*this, hit);
				}

				//recalculate end point etc so visuals don't go through
				xform.position = start;
				stretchToDistance(hitDistance, false, currentLevel);

				//make sure this projectile will expire on next tick!
				hitLocation = hit;
				bHit = true;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile system
	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProjectileSystem::postGameLoopTick(float system_dt_sec)
	{
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			const sp<TimeManager>& worldTM = currentLevel->getWorldTimeManager();

			if (!worldTM->isTimeFrozen())
			{
				auto iter = std::begin(activeProjectiles);
				auto end = std::end(activeProjectiles);
				while (iter != end)
				{
					//since projectiles may expire, we want need to remove them from the set of active projectiles
					//we can do this while iterating over std::set, but we need to take care. Removing an interator
					//only invalidates that iterator, but if it is invalidated we cannot get the next iterator from it.
					//So, we copy the iterator and immediately get the next iterator; meaning removing the copy doesn't
					//affect our ability to get the next iterator
					auto iterCopy = iter;
					++iter; 
					
					const sp<Projectile>& projectile = *iterCopy;
					projectile->tick(worldTM->getDeltaTimeSecs(), *currentLevel);

					if (projectile->timeAlive > projectile->lifetimeSec || projectile->forceRelease)
					{
						if (projectile->soundEmitter)
						{
							projectile->soundEmitter->stop();
							sfxPool.releaseInstance(projectile->soundEmitter);
							projectile->soundEmitter = nullptr;
						}

						if (projectile->pointLight)
						{
							lightPool.releaseInstance(projectile->pointLight);
							projectile->pointLight = nullptr;
						}
				
						//note: this projectile will keep any sp alive, so clear before release if needed
						objPool.releaseInstance(projectile);

						//removing iterator from set does not invalidate other iterators; 
						//IMPORANT: this must after releasing to pool, otherwise the smart pointer will be deleted
						activeProjectiles.erase(iterCopy); //#TODO #performance swap and pop will probably greatly speed this up; there might be a reason we need it in a set though
					}
				}
			}
		}
	}

	void ProjectileSystem::handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		sfxPool.clear();
	}

	void ProjectileSystem::handleRenderDispatch(float dtSec)
	{
		RenderSystem& renderSystem = GameBase::get().getRenderSystem();
		if (const RenderData* frd = renderSystem.getFrameRenderData_Read(GameBase::get().getFrameNumber()))
		{
			if (DeferredRendererStateMachine* deferredRenderer = renderSystem.getDeferredRenderer())
			{
				if (deferedShaded_EmissiveModelShader)
				{
					deferedShaded_EmissiveModelShader->use();
					deferredRenderer->configureShaderForGBufferWrite(*deferedShaded_EmissiveModelShader);
					renderProjectiles(*deferedShaded_EmissiveModelShader);
				}
				else { STOP_DEBUGGER_HERE(); }
			}
			else
			{
				//forward shading
				if (forwardShaded_EmissiveModelShader)
				{
					forwardShaded_EmissiveModelShader->use();
					forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(frd->view));
					forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(frd->projection));
					renderProjectiles(*forwardShaded_EmissiveModelShader);
				}
				else { STOP_DEBUGGER_HERE(); }
			}
		}
	}

	void ProjectileSystem::initSystem()
	{
		//align projectiles with camera
		GameBase::get().onPostGameloopTick.addStrongObj(sp_this(), &ProjectileSystem::postGameLoopTick);
		GameBase::get().getLevelSystem().onPostLevelChange.addWeakObj(sp_this(), &ProjectileSystem::handlePostLevelChange);

		GameBase::get().onRenderDispatch.addWeakObj(sp_this(), &ProjectileSystem::handleRenderDispatch);

		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);
		deferedShaded_EmissiveModelShader = new_sp<SA::Shader>(gbufferShader_vs, gbufferShader_emissive_fs, false);

		//have pools reserve underlying memory for estimates on how many we expect to be in pool concurrently
		size_t estimateNumberConcurrentProjectiles = 300;
		objPool.reserve(estimateNumberConcurrentProjectiles);
		sfxPool.reserve(estimateNumberConcurrentProjectiles);
		lightPool.reserve(estimateNumberConcurrentProjectiles);
	}

	void ProjectileSystem::spawnProjectile(const ProjectileSystem::SpawnData& spawnData, const ProjectileConfig& projectileTypeHandle)
	{
		sp<Projectile> spawned = objPool.getInstance();

		//#optimize note there may some optimized functions in glm to do this work
		glm::vec3 projectileSystemForward(0, 0, -1);
		glm::quat spawnRotation = Utils::getRotationBetween(projectileSystemForward, spawnData.direction_n);

		spawned->xform.rotQuat = spawnRotation;
		spawned->xform.position = spawnData.start;
		spawned->damage = spawnData.damage;
		spawned->color = spawnData.color * (GameBase::get().getRenderSystem().isUsingHDR() ? 4.f : 1.f); //make color glow if using HDR //@hdr_tweak
		spawned->team = spawnData.team;
		spawned->owner = spawnData.owner;
		spawned->direction_n = spawnData.direction_n;
		spawned->renderXform = glm::scale(glm::mat4(1.f), { 0, 0, 0 });

		spawned->bHit = false;
		spawned->forceRelease = false;

		spawned->distanceStretchScale = 1;
		spawned->directionQuat = spawnRotation;

		spawned->speed = projectileTypeHandle.getSpeed();
		spawned->model = projectileTypeHandle.getModel();
		spawned->lifetimeSec = projectileTypeHandle.getLifetimeSecs();
		spawned->aabbSize = projectileTypeHandle.getAABBsize();

		spawned->soundEmitter = spawnSfxEffect(spawnData.sfx, spawnData.start);
		spawned->pointLight = spawnPointLight(spawnData);

		spawned->bCorrectPosition = spawnData.traceStart.has_value();
		spawned->traceStartPos = spawnData.traceStart.value_or(glm::vec3(0.f));
		spawned->offsetTraceCorrectionDistance = 30.f; //hardcoded for now, perhaps should be tweakable per model as size of model may vary
		spawned->offsetStartPos = spawned->xform.position;
		
		spawned->timeAlive = 0.f;
#if _WIN32 && _DEBUG
		if (Utils::anyValueNAN(spawnRotation)) { __debugbreak(); return; }
		if (Utils::anyValueNAN(spawnData.start)) { __debugbreak(); return; }
#endif

		activeProjectiles.insert( spawned );
	}

	void ProjectileSystem::unspawnAllProjectiles()
	{
		activeProjectiles.clear();
	}

	void ProjectileSystem::renderProjectiles(Shader& projectileShader) const
	{
		//#optimize potential optimization is to use instanced rendering to reduce draw call number
		//#TODO perhaps projectile should be made a full class and encapsulate this logic
		//#TODO refactor so projectile system is self-sufficient and doesn't rely on Game to call "render". 
		//#TODO refactor so instance rendered, set of uniforms can define instance

		//invariant: shader uniforms pre-configured
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			projectileShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectile->renderXform));
			projectileShader.setUniform3f("lightColor", projectile->color); //#TODO this uniform name is very specific to emissive shader; either need a callback to configure uniforms unique to projectile or move shader here; NOTE: this perf loss scales linearly, adding this one uniform drops fps by 0.1
			projectile->model->draw(projectileShader, false); //not binding materials projectiles don't use materials and this is causing a gl error when attempting ot bind a normal map texture
		}
	}

	void ProjectileSystem::renderProjectileBoundingBoxes(Shader& debugShader, const glm::vec3& color, const glm::mat4& view, const glm::mat4& perspective) const
	{
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			Utils::renderDebugWireCube(debugShader, color, projectile->collisionXform, view, perspective);
		}
	}

	sp<SA::AudioEmitter> ProjectileSystem::spawnSfxEffect(const SoundEffectSubConfig& sfx, glm::vec3 position)
	{
		if (sfx.assetPath.size() != 0)
		{
			sp<AudioEmitter> recycledEmitter = sfxPool.getInstance();
			if (!recycledEmitter)
			{
				recycledEmitter = GameBase::get().getAudioSystem().createEmitter();
			}
			//clean up recycled emitter
			recycledEmitter->stop();
			sfx.configureEmitter(recycledEmitter);
			recycledEmitter->setPriority(AudioEmitterPriority::GAMEPLAY_COMBAT);
			recycledEmitter->setPosition(position);
			recycledEmitter->play();
			return recycledEmitter;
		}
		return nullptr;
	}

	sp<PointLight_Deferred> ProjectileSystem::spawnPointLight(const ProjectileSystem::SpawnData& spawnData)
	{
#if IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
		return nullptr;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

		if (bEnableProjectilePointLights)
		{
			if (spawnData.projectileLightData.has_value())
			{
				sp<PointLight_Deferred> recycledLight = lightPool.getInstance();
				if (!recycledLight)
				{
					recycledLight = GameBase::get().getRenderSystem().createPointLight();
				}
				PointLight_Deferred::UserData& mutableUserData = recycledLight->getMutableUserData();
				mutableUserData = *spawnData.projectileLightData;
				mutableUserData.position = spawnData.start;
				mutableUserData.diffuseIntensity = spawnData.color; //TODO perhaps scale this up by some factor for blurring etc
				mutableUserData.bActive = true;
				return recycledLight;
			}
		}
		return nullptr;
	}

}

