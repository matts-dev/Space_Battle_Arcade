#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/color_utils.h"
#include "../../Rendering/SAGPUResource.h"

namespace SA
{
	class SphereMeshTextured;
	class Shader;

	struct Stars
	{
		std::vector<glm::mat4> xforms;
		std::vector<glm::vec3> colors;
	};

	class StarField : public GPUResource
	{
	public:
		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection);

		bool getForceCentered() { return bGetForceCentered; }
		void setForceCentered(bool bNewForceCentered) { bGetForceCentered = bNewForceCentered; }
	protected:
		virtual void postConstruct() override;
	private:
		virtual void onReleaseGPUResources() override;
		virtual void onAcquireGPUResources() override;
		void handleAcquireInstanceVBOOnNextTick(); 
		void bufferInstanceData();
		void generateStarField();

	private:
		sp<SphereMeshTextured> starMesh;
		sp<MultiDelegate<>> timerDelegate;
		sp<Shader> starShader;

		bool bUseHDR = false;
		bool bGenerated = false;
		bool bDataBuffered = false;
		bool bGetForceCentered = true;
		uint32_t numStars = 50000;
		uint32_t seed = 27;
		std::array<glm::vec3, 3> colorScheme = { color::lightYellow(), color::red(), color::blue() };

		Stars stars;

	private: //gpu resources
		uint32_t modelVBO;
		uint32_t colorVBO;


	};

}