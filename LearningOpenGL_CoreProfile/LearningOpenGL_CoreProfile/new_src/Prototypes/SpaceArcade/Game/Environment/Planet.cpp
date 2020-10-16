#include <assert.h>
#include "Planet.h"
#include "../../Tools/ModelLoading/SAModel.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../Rendering/Camera/Texture_2D.h"
#include "../../GameFramework/SALevel.h"
#include "../../Rendering/Lights/SADirectionLight.h"
#include "../../Rendering/RenderData.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../GameFramework/SAAssetSystem.h"

namespace SA
{
	using namespace glm;

	static char const* const planetShader_vs= R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 vertUV; 

		out vec2 uv;
		out vec3 fragNormal_raw;

		uniform mat4 projection_view;
		uniform mat4 model;
		uniform vec3 starColor;

		void main(){
			gl_Position = projection_view * model * vec4(position, 1.0f);

			mat4 normalMat = inverse(transpose(model));
			fragNormal_raw = normalize(vec3(normalMat * vec4(normal,0)));
			uv = vertUV;
		}
	)";

	static char const* const planetShader_fs = R"(
		#version 330 core
		out vec4 fragColor;

		in vec2 uv;
		in vec3 fragNormal_raw;

		uniform sampler2D albedo_0;
		uniform sampler2D albedo_1;
		uniform sampler2D albedo_2;
		uniform sampler2D colorMap;

		//city lights will match the terrain albedo (ie slot associated with green)
		uniform int bEnableCityLights = 0;
		uniform int bLuminosityGrayScale = 0;
		uniform sampler2D cityLights;

		uniform vec3 ambientLight = vec3(0.f);

		struct DirectionLight
		{
			vec3 dir_n;
			vec3 intensity;
		};	
		#define MAX_DIR_LIGHTS 4
		uniform DirectionLight dirLights[MAX_DIR_LIGHTS];

		void main(){
			vec4 alb_0 = texture(albedo_0, uv);
			vec4 alb_1 = texture(albedo_1, uv);
			vec4 alb_2 = texture(albedo_2, uv);
			vec4 features = texture(colorMap, uv);

			vec4 texColor = alb_0;
			texColor = mix(texColor, alb_1, features.g);
			texColor = mix(texColor, alb_2, features.b);

			vec3 color = vec3(0.f);
			float summedLightFrac = 0.f;
			vec3 fragNormal_n = normalize(fragNormal_raw);
			for(int light = 0; light < MAX_DIR_LIGHTS; ++light)
			{
				float n_dot_l = max(dot(fragNormal_n, -dirLights[light].dir_n), 0.f);
				summedLightFrac += n_dot_l;
				color += texColor.rgb * n_dot_l * dirLights[light].intensity;
			}

			float darkEnoughForCityLight = 0.25f; //if there is less than X light on the range [0,1] (ie dot product) then fill in some city light. Note this range is actually larger than [0,1] since we're summing multiple dir lights
			if(bEnableCityLights > 0 && summedLightFrac < darkEnoughForCityLight)
			{
				vec4 cityLightTex = texture(cityLights, uv);

				//filter out the darker colors (ie the night blue). We're using lengths of colors, which may seem strange.
				// But it is a good proxy for how bright an SDR color is. len(vec3(1,1,1)) == 1.732, len(vec3(0.25f, 0.1, 0.1)) == 0.287
				// doing this smoothly (eg as fraction) should avoid a jagged edge
				float cityColorLength = length(cityLightTex.rgb);
				float textureCutoffThreshold = 0.25f;
				float adjColorLen = clamp(cityColorLength - textureCutoffThreshold, 0, cityColorLength);
				float colorRampUp = adjColorLen / length(vec3(1,1,1));

				float darkFraction = 1.0f - (summedLightFrac  / darkEnoughForCityLight);	// [0,X] -> [0,1] -> [1,0]
				float dampeningFactor = 1.0f;
				float landFactor = features.g;
				color += darkFraction * dampeningFactor * landFactor * colorRampUp * cityLightTex.rgb;	//suggested perhaps make this part emissive by scaling up city light texture.
			}

			color += ambientLight * texColor.rgb;

		#define TONE_MAP_HDR 0
		#if TONE_MAP_HDR
			vec3 reinHardToneMapped = (color) / (1 + color);
			fragColor = vec4(reinHardToneMapped, 1.f);
		#else
			fragColor = vec4(color, 1.f);
		#endif

			if(bLuminosityGrayScale > 0) 
			{
				//fragColor = vec4(vec3(fragColor.r*0.3 + fragColor.g*0.59 + fragColor.b*0.11), 1.f); //based on tutorial point for converting RGB to grayscale
				fragColor = vec4(vec3(fragColor.r + fragColor.g+ fragColor.b)/3.f, 1.f); //non-luminosity version
			}
		}
	)";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Statics
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	sp<Shader> Planet::planetShader;
	sp<SA::Model3D> Planet::planetModel;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Instance Members
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Planet::applySizeCorrections()
	{
		data.xform.scale *= 10;
	}

	void Planet::setOverrideData(Planet::Data& data)
	{
		this->data = data;

		applySizeCorrections();

		AssetSystem& assetSystem = GameBase::get().getAssetSystem();

		planetModel = planetModel ? planetModel : assetSystem.loadModel("GameData/mods/SpaceArcade/Assets/Models3D/Planet/textured_planet.obj");
		planetShader = planetShader ? planetShader : new_sp<Shader>(planetShader_vs, planetShader_fs, false);

		//#TODO have these load textures from asset system once shared configured textures(mips, etc) are a thing 
		assert(data.albedo1_filepath.has_value());
		albedo0Tex = data.albedo1_filepath.has_value() ? new_sp<Texture_2D>(data.albedo1_filepath.value()) : nullptr;
		albedo1Tex = data.albedo2_filepath.has_value() ? new_sp<Texture_2D>(data.albedo2_filepath.value()) : nullptr;
		albedo2Tex = data.albedo3_filepath.has_value() ? new_sp<Texture_2D>(data.albedo3_filepath.value()) : nullptr;
		citylightTex = data.nightCityLightTex_filepath.has_value() ? new_sp<Texture_2D>(data.nightCityLightTex_filepath.value()) : nullptr;
		colorMapTex = data.colorMapTex_filepath.has_value() ? new_sp<Texture_2D>(data.colorMapTex_filepath.value()) : nullptr;
		nullBlackTex = assetSystem.getNullBlackTexture();
	}

	void Planet::postConstruct()
	{
		setOverrideData(data);
	}

	void Planet::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)

	{
		if (!albedo0Tex) { return; }
		static GameBase& game = GameBase::get();

		planetShader->use();
		
		//#SUGGESTED a lot of this code is similar to star.cpp, perhaps a base class "celestial body" can work. Depends on how planets map movement.
		//#TODO perhaps slightly move the planet when moving towards it? will need to map large distances to small changes
		mat4 model = data.xform.getModelMatrix();

		mat4 customView = view;
		vec3 camFront_n = vec3(1.f, 0.f, 0.f);
		PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSys.getPlayer(0);
		if (const sp<CameraBase>& camera = player ? player->getCamera() : nullptr)
		{
			if (bUseLargeDistanceApproximation)
			{
				vec3 origin{ 0.f };
				customView = glm::lookAt(origin, origin + camera->getFront(), camera->getUp());
			}
			camFront_n = camera->getFront();

			sj.tick(dt_sec);
			if (sj.isStarJumpInProgress())
			{
				//slide the plant along camera direction if we're doing a star jump
				float slideFactor = 4000.f; //balanced with LOCAL star slide factor so that planets move faster than local stars
				Transform slideXform = data.xform;
				slideXform.position += -camFront_n * sj.starJumpPerc * slideFactor;
				model = slideXform.getModelMatrix();
			}
		}
		mat4 projection_view = projection * customView;

		planetShader->use();
		planetShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		planetShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));

		planetShader->setUniform1i("albedo_0", 0);
		albedo0Tex->bindTexture(GL_TEXTURE0);

		bool bUseMultiTexturePaint = albedo1Tex && albedo2Tex && colorMapTex;
		bool bUseCityLightTexture = bool(citylightTex);

		planetShader->setUniform1i("bUseMultiTexturedPlanet", int32_t(bUseMultiTexturePaint)); //#optimize this can be optimized so it isn't done every frame, if we don't share a shader at least
		planetShader->setUniform1i("bEnableCityLights", int32_t(bUseCityLightTexture));
		planetShader->setUniform1i("bLuminosityGrayScale", int32_t(bUseGrayScale));
		if (bUseMultiTexturePaint)
		{
			albedo1Tex->bindTexture(GL_TEXTURE1);
			planetShader->setUniform1i("albedo_1", 1);

			albedo2Tex->bindTexture(GL_TEXTURE2);
			planetShader->setUniform1i("albedo_2", 2);

			colorMapTex->bindTexture(GL_TEXTURE3);
			planetShader->setUniform1i("colorMap", 3);
		}
		else
		{
			//clear out any previous data
			nullBlackTex->bindTexture(GL_TEXTURE1);
			planetShader->setUniform1i("albedo_1", 1);
			planetShader->setUniform1i("albedo_2", 1);
			planetShader->setUniform1i("colorMap", 1);
		}

		if (bUseCityLightTexture)
		{
			citylightTex->bindTexture(GL_TEXTURE4);
			planetShader->setUniform1i("cityLights", 4);
		}
		else
		{
			//clear out any previous data
			nullBlackTex->bindTexture(GL_TEXTURE4);
			planetShader->setUniform1i("cityLights", 4);
		}


		if (const sp<LevelBase>& loadedLevel = game.getLevelSystem().getCurrentLevel())
		{
			if (const RenderData* FRD = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
			{
				planetShader->setUniform3f("ambientLight", FRD->ambientLightIntensity);

				size_t lightIdx = 0;
				if (!bUseCameraLight) //environment mode
				{
					for (const DirectionLight& light : FRD->dirLights)
					{
						light.applyToShader(*planetShader, lightIdx++);
					}
				}
				else //camera lit (UI mode)
				{
					DirectionLight uiLight;
					uiLight.lightIntensity = vec3(1.f);
					uiLight.direction_n = camFront_n;
					uiLight.applyToShader(*planetShader, 0);
				}
			}
		}

		planetModel->draw(*planetShader, false);
	}

	bool Planet::tick(float dt_sec)
	{
		data.xform.rotQuat;

		if (data.rotSpeedSec_rad != 0.0f)
		{
			data.xform.rotQuat = glm::angleAxis(data.rotSpeedSec_rad * dt_sec, data.rotationAxis) * data.xform.rotQuat;
		}

		return true;
	}

	void Planet::setUseCameraAsLight(bool bInUseCameraLight)
	{
		bUseCameraLight = bInUseCameraLight;
	}

	void Planet::enableStarJump(bool bEnable, bool bSkipTransition /*= false*/)
	{
		sj.enableStarJump(bEnable, bSkipTransition);
	}

	void Planet::updateTransformForData(glm::vec3 offsetDir, float offsetDist)
	{
		offsetDir = glm::normalize(offsetDir);

		Transform newXform = {};
		newXform.position = offsetDir * offsetDist;
		setTransform(newXform);
	}

	sp<class Planet> makeRandomPlanet(RNG& rng)
	{
		static const std::vector<std::string> defaultTextures = {
			std::string( DefaultPlanetTexturesPaths::albedo1  ),
			std::string( DefaultPlanetTexturesPaths::albedo2  ),
			std::string( DefaultPlanetTexturesPaths::albedo3  ),
			std::string( DefaultPlanetTexturesPaths::albedo4  ),
			std::string( DefaultPlanetTexturesPaths::albedo5  ),
			std::string( DefaultPlanetTexturesPaths::albedo6  ),
			std::string( DefaultPlanetTexturesPaths::albedo7  ),
			std::string( DefaultPlanetTexturesPaths::albedo8  ),
			std::string( DefaultPlanetTexturesPaths::albedo_terrain  ),
			std::string( DefaultPlanetTexturesPaths::albedo_sea ),
			std::string( DefaultPlanetTexturesPaths::albedo_nightlight )
		};

		uint32_t choice = rng.getInt<uint32_t>(0, 11);

		Planet::Data init;
		if (choice == 0)
		{
			//use the multi-layer material example
			init.albedo1_filepath = DefaultPlanetTexturesPaths::albedo_sea;
			init.albedo2_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
			init.albedo3_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
			init.nightCityLightTex_filepath = DefaultPlanetTexturesPaths::albedo_nightlight;
			init.colorMapTex_filepath = DefaultPlanetTexturesPaths::colorChanel;
		}
		else
		{
			choice -= 1; //make it zero based (0 counted towards the planet example)
			assert( (choice < defaultTextures.size()) );
			init.albedo1_filepath = defaultTextures[choice];
		}

		float distance = rng.getFloat(50.0f, 100.0f);
		vec3 pos{ rng.getFloat(-20.f, 20.f), rng.getFloat(-20.f, 20.f), rng.getFloat(-20.f, 20.f) };
		pos.x = (pos.x == 0) ? 0.1f : pos.x; //make sure normalization cannot divide by 0, which will create nans
		pos = normalize(pos + vec3(0.1f)) * distance;
		init.xform.position = pos;

		return new_sp<Planet>(init);
	}

}

