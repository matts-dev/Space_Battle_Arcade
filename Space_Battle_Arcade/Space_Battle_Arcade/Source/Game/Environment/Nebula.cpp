#include "Nebula.h"
#include "../../Tools/Geometry/SimpleShapes.h"
#include "../../Rendering/RenderData.h"
#include "Rendering/SAShader.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "GameFramework/SAGameBase.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "Tools/SAUtilities.h"
#include "../../Rendering/Camera/Texture_2D.h"
#include "../SpaceArcade.h"
#include "../GameSystems/SAModSystem.h"

namespace SA
{
	using namespace glm;

	char const* const nebula_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec2 inTexCoord;		
				
		out vec2 texCoord;

		uniform mat4 model;
		uniform mat4 projection_view;

		void main(){
			texCoord = inTexCoord;
			gl_Position = projection_view * model * vec4(position, 1.f);
		}
	)";
	char const* const nebula_fs = R"(
		#version 330 core

		out vec4 fragmentColor;
		in vec2 texCoord;

		uniform sampler2D textureData;   
		uniform vec3 uColor = vec3(1.f);
		uniform float uSuppressionPerc = 0.f;

		void main()
		{
			vec3 textureColor = texture(textureData, texCoord).rgb;
			vec3 color = uColor * textureColor;
			float alphaInfluence = 1.f;

			vec2 centeredUV = abs(texCoord - 0.5f) * 2;
			float uvLength = length(centeredUV);
			alphaInfluence = 1.f - clamp(uvLength, 0.f, 1.f); //invert range

			fragmentColor = vec4(color, min(1.f - uSuppressionPerc, alphaInfluence));  //use this when blending
			//fragmentColor = vec4(color * min(1.f - uSuppressionPerc, alphaInfluence), 1.f); //use this when not blending
		}
	)";


	/*static*/ sp<SA::TexturedQuad> Nebula::texturedQuad = nullptr;
	/*static*/ sp<Shader> Nebula::circleFadeShader = nullptr;

	Nebula::Nebula(Data inData) : data(inData)
	{
	}

	void Nebula::postConstruct()
	{
		Parent::postConstruct();

		{ //init statics
			if (!texturedQuad)
			{
				//can't do static initialization because game base isn't around, also probably bad to rely on static init
				texturedQuad = new_sp<TexturedQuad>();
			}
			if (!circleFadeShader)
			{
				circleFadeShader = new_sp<Shader>(nebula_vs, nebula_fs, false);
			}
		}

		//feels weird to do this here, but override data makes sure there is a difference as an optimization... so doing this manually.
		const sp<ModSystem>& modSystem = SpaceArcade::get().getModSystem();
		const sp<Mod>& activeMod = modSystem->getActiveMod();
		if (data.textureRelativePath.length() > 0 && activeMod)
		{
			nebulaTexture = new_sp<Texture_2D>(activeMod->getModDirectoryPath() + data.textureRelativePath);
		}
	}

	//void Nebula::primeShaderForRender(const RenderData& rd)
	//{
	//	transparentShader->use();
	//	todo_set_uniforms;
	//}

	void Nebula::rotateRelativeToOrigin()
	{
		quat rot = Utils::getRotationBetween(vec3(0, 0, 1), normalize(xform.position));
		xform.rotQuat = rot;
	}

	void Nebula::render(const RenderData& rd)
	{
		circleFadeShader->use();
		float starJumpSupressionPerc = 0.f;

		mat4 model = xform.getModelMatrix();
		mat4 customView = rd.view;
		PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSys.getPlayer(0);
		if (const sp<CameraBase>& camera = player ? player->getCamera() : nullptr)
		{
			if (bForceCentered)
			{
				vec3 origin{ 0.f };
				customView = glm::lookAt(origin, origin + camera->getFront(), camera->getUp());
			}

			sj.tick(rd.dt_sec);
			if (sj.isStarJumpInProgress())
			{
				float maxStarjumpPercBeforeInvisible = 0.05f;
				starJumpSupressionPerc = glm::clamp(sj.starJumpPerc/maxStarjumpPercBeforeInvisible, 0.f, 1.f);
			}
		}

		mat4 projection_view = rd.projection * customView;
		circleFadeShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		circleFadeShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view)); //#todo move this to shader prime function
		circleFadeShader->setUniform3f("uColor", data.tintColor);
		circleFadeShader->setUniform1f("uSuppressionPerc", starJumpSupressionPerc);
		circleFadeShader->setUniform1i("textureData", 0);

		if (nebulaTexture)
		{
			nebulaTexture->bindTexture(GL_TEXTURE0);
		}

		texturedQuad->render();
	}

	void Nebula::setOverrideData(Nebula::Data& overrideData)
	{
		if (overrideData.textureRelativePath != data.textureRelativePath)
		{
			//only make a texture if something changed, since this may or may not be called frequently
			const sp<ModSystem>& modSystem = SpaceArcade::get().getModSystem();
			const sp<Mod>& activeMod = modSystem->getActiveMod();
			if (activeMod)
			{
				nebulaTexture = new_sp<Texture_2D>(activeMod->getModDirectoryPath() + overrideData.textureRelativePath);
			}
		}
		data = overrideData;
		if (hasAcquiredResources())
		{
			//force recalculation of texture
			onReleaseGPUResources();
			onAcquireGPUResources();
		}
	}

	void Nebula::setXform(Transform inXform, bool bRotateToOrigin)
	{
		this->xform = inXform;

		if (bRotateToOrigin)
		{
			rotateRelativeToOrigin();
		}
	}

	void Nebula::onAcquireGPUResources()
	{
		//Parent::onAcquireGPUResources();
		GameBase::get().getAssetSystem().loadTexture(data.textureRelativePath.c_str(), nebulaTex);
	}

	void Nebula::onReleaseGPUResources()
	{
		//Parent::onReleaseGPUResources();
	}

}

