#include <algorithm>

#include "Star.h"

#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAPlayerBase.h"
#include "GameFramework/SAPlayerSystem.h"
#include "Rendering/Camera/SACameraBase.h"
#include "Rendering/SAShader.h"
#include "Tools/Geometry/SimpleShapes.h"

namespace SA
{
	using namespace glm;

	static char const* const localStarShader_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 uv; //necessary, but comes with the mesh so leaving for quick tweaks to add textures

		uniform mat4 projection_view;
		uniform mat4 model;
		uniform vec3 starColor;

		out vec3 color;

		void main(){
			gl_Position = projection_view * model * vec4(position, 1.0f);
			color = starColor;
		}
	)";

	static char const* const localStarShader_fs = R"(
		#version 330 core
		out vec4 fragColor;

		in vec3 color;

		void main(){
		#define TONE_MAP_HDR 0
		#if TONE_MAP_HDR
			vec3 reinHardToneMapped = (color) / (1 + color);
			fragColor = vec4(reinHardToneMapped, 1.0f);
		#else
			fragColor = vec4(color, 1.0f);
		#endif
		}
	)";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Statics
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	sp<SphereMeshTextured> Star::starMesh = nullptr;
	sp<Shader> Star::starShader = nullptr;
	StarContainer Star::sceneStars;
	uint32_t Star::starInstanceCount = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Instances
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Star::Star()
	{
		++starInstanceCount;
		if (!starMesh) { starMesh = new_sp<SphereMeshTextured>(); }
		if (!starShader) { starShader = new_sp<Shader>(localStarShader_vs, localStarShader_fs, false); }
	}

	Star::~Star()
	{
		--starInstanceCount;
		if (starInstanceCount == 0)
		{
			starMesh = nullptr;
			starShader = nullptr;
		}

		auto newEndIter = std::remove_if(sceneStars.begin(), sceneStars.end(),
			[this](const fwp<const Star>& star)
			{ 
				return star.get() == this //remove this form list
					|| !bool(star); //also clean up any stars that have been nulled out.
			});
		sceneStars.erase(newEndIter, sceneStars.end());
	}

	void Star::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		mat4 model = xform.getModelMatrix();
		mat4 customView = view;
		PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSys.getPlayer(0);
		if (const sp<CameraBase>& camera = player ? player->getCamera() : nullptr)
		{
			if (bForceCentered)
			{
				vec3 origin{ 0.f };
				customView = glm::lookAt(origin, origin + camera->getFront(), camera->getUp());
			}

			sj.tick(dt_sec);
			if (sj.isStarJumpInProgress())
			{
				glm::vec3 camFront_n = camera->getFront();

				//slide the star along camera direction if we're doing a star jump
				float slideFactor = 500.f;
				Transform slideXform = xform;
				slideXform.position += -camFront_n * sj.starJumpPerc * slideFactor;
				model = slideXform.getModelMatrix();
			}

		}
		mat4 projection_view = projection * customView;

		starShader->use();
		starShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		starShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));
		starShader->setUniform3f("starColor", bUseHDR ? ldr_color * hdr_intensity : ldr_color);
		starMesh->render();
	}

	void Star::postConstruct()
	{
		sceneStars.push_back(sp_this());
		if (sceneStars.size() > MAX_STAR_DIR_LIGHTS)
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Too many star lights instantiated");
		}
	}

	glm::vec3 Star::getLightDirection() const
	{
		//direction from star towards origin
		return vec3(0, 0, 0) - xform.position;
	}

	void Star::enableStarJump(bool bEnable, bool bSkipTransition /*= false*/)
	{
		sj.enableStarJump(bEnable, bSkipTransition);
	}

	void Star::updateXformForData(glm::vec3 starDir, float starDist)
	{
		starDir = glm::normalize(starDir);

		Transform starXform = {};
		starXform.position = starDir * starDist;
		setXform(starXform);
	}


}
