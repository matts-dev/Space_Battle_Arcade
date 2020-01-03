#include "SASpaceLevelBase.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../SAProjectileSystem.h"
#include "../Team/Commanders.h"
#include "../Environment/StarField.h"
#include "../Environment/Star.h"
#include "../../Rendering/OpenGLHelpers.h"

namespace SA
{

	SpaceLevelBase::SpaceLevelBase()
	{
	}

	void SpaceLevelBase::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		const sp<PlayerBase>& zeroPlayer = GameBase::get().getPlayerSystem().getPlayer(0);
		if (zeroPlayer)
		{
			const sp<CameraBase>& camera = zeroPlayer->getCamera();

			if (starField)
			{
				starField->render(dt_sec, view, projection);
			}

			//stars may be overlapping each other, so only clear depth once we've rendered all the solar system stars.
			ec(glClear(GL_DEPTH_BUFFER_BIT));
			for (const sp<Star>& star : localStars)
			{
				star->render(dt_sec, view, projection);
			}
			ec(glClear(GL_DEPTH_BUFFER_BIT));

			//#todo a proper system for renderables should be set up; these uniforms only need to be set up front, not during each draw. It may also be advantageous to avoid virtual calls.
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0)); //perhaps drive this from level information

			forwardShadedModelShader->setUniform3f("cameraPosition", camera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);
			for (const sp<RenderModelEntity>& entity : renderEntities) 
			{
				forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
				entity->draw(*forwardShadedModelShader);
			}
		}
	}

	TeamCommander* SpaceLevelBase::getTeamCommander(size_t teamIdx)
	{
		return teamIdx < commanders.size() ? commanders[teamIdx].get() : nullptr;
	}

	void SpaceLevelBase::startLevel_v()
	{
		LevelBase::startLevel_v();

		forwardShadedModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_SimpleLighting_fragSrc, false);

		for (size_t teamId = 0; teamId < numTeams; ++teamId)
		{
			commanders.push_back(new_sp<TeamCommander>(teamId));
		}
	}

	void SpaceLevelBase::endLevel_v()
	{
		//this means that we're going to generate a new shader between each level transition.
		//this can be avoided with static members or by some other mechanism, but I do not see 
		//transitioning levels being a slow process currently, so each level gets its own shaders.
		forwardShadedModelShader = nullptr;

		LevelBase::endLevel_v();
	}

	void SpaceLevelBase::postConstruct()
	{
		starField = onGenerateStarField();
		{ 
			bGeneratingLocalStars = true;
			onGenerateLocalStars();
			bGeneratingLocalStars = false;
		}
	}

	sp<SA::StarField> SpaceLevelBase::onGenerateStarField()
	{
		//by default generate a star field, if no star field should be in the level return null in an override;
		sp<StarField> defaultStarfield = new_sp<StarField>();
		return defaultStarfield;
	}

	void SpaceLevelBase::onGenerateLocalStars()
	{
		Transform defaultStarXform;
		defaultStarXform.position = glm::vec3(50, 50, 50);

		sp<Star> defaultStar = new_sp<Star>();
		defaultStar->setXform(defaultStarXform);
		addStar(defaultStar);
	}

	void SpaceLevelBase::addStar(const sp<Star>& newStar)
	{
		if (bGeneratingLocalStars)
		{
			auto foundIter = std::find_if(localStars.begin(), localStars.end(), [newStar](const sp<Star>& star) {return newStar == star; });
			if (foundIter == localStars.end())
			{
				localStars.push_back(newStar);
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "trying to add duplicate star");
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "trying to generate stars outside of level initialziation");
		}
	}

	sp<SA::StarField> SpaceLevelBase::getStarField()
	{
		return starField;
	}

}

