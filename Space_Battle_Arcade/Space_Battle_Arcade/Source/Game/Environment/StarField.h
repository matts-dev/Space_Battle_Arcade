#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "GameFramework/SAGameEntity.h"
#include "Tools/DataStructures/SATransform.h"
#include "Tools/color_utils.h"
#include "Rendering/SAGPUResource.h"
#include "Game/Environment/StarJumpData.h"
#include <optional>

namespace SA
{
	class SphereMeshTextured;
	class Shader;
	class TexturedQuad;
	class Texture_2D;
	class AudioEmitter;

	struct Stars
	{
		std::vector<glm::mat4> xforms;
		std::vector<glm::vec3> colors;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the environmental night sky of stars. Generates 3d points for stars and renders them using sphere meshes.
	// Doing so in this way, rather than using a texture, allows for certain effects that require time exposure and 3d positions.
	// stars use instanced rendering for performance.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class StarField final : public GPUResource
	{
	public:
		struct InitData 
		{
			std::optional<std::array<glm::vec3, 3>> colorScheme = std::nullopt;
		};
	public:
		StarField(const StarField::InitData& init = InitData());
		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection);

		bool getForceCentered() { return bForceCentered; }
		void setForceCentered(bool bNewForceCentered) { bForceCentered = bNewForceCentered; }
		void enableStarJump(bool bEnable, bool bSkipTransition = false);
		bool isStarJumping() { return sj.isStarJumpInProgress(); }
		std::array<glm::vec3, 3>& getColorScheme() { return colorScheme; }
		void regenerate(); //used to regenerate star field in level editor when colors are tweaked
	protected:
		virtual void postConstruct() override;
	private:
		virtual void onReleaseGPUResources() override;
		virtual void onAcquireGPUResources() override;
		void handleAcquireInstanceVBOOnNextTick(); 
		void bufferInstanceData();
		void generateStarField();
		void updateStarJumpSFX();
	private://impl
		StarJumpData sj;
		bool bStarJumpBoomPlayed = false;
		bool bStarJumpSfxComplete = false;
		sp<TexturedQuad> texturedQuad = nullptr;
		sp<Texture_2D> starJumpTexture;
		sp<AudioEmitter> sfx_starJumpWindUp = nullptr;
		sp<AudioEmitter> sfx_starJumpWindDown = nullptr;
		sp<AudioEmitter> sfx_starJumpBoom = nullptr;
	private:
		sp<SphereMeshTextured> starMesh;
		sp<MultiDelegate<>> timerDelegate;
		sp<Shader> starShader;
		sp<Shader> spiralShader;
		bool bUseHDR = true;
		bool bGenerated = false;
		bool bDataBuffered = false;
		bool bForceCentered = true;
		uint32_t numStars = 50000;
		uint32_t seed = 27;
		std::array<glm::vec3, 3> colorScheme = { color::lightYellow(), color::red(), color::blue() };

		Stars stars;

	private: //gpu resources
		uint32_t modelVBO;
		uint32_t colorVBO;
	};

}