#pragma once

#include <cstdint>
#include <vector>
#include "../SpaceArcade.h"
#include "../SASpaceArcadeGlobalConstants.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/AdvancedPtrs.h"


namespace SA
{
	//forward delcarations
	class SphereMeshTextured;
	class Shader;

	using StarContainer = std::vector<fwp<const class Star>>;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents a local star to the solar system; these influence the directional lighting of a scene
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Star final : public GameEntity, public RemoveCopies
	{
	public:
		const uint32_t MAX_STAR_DIR_LIGHTS = GlobalConstants::MAX_DIR_LIGHTS;
		static const StarContainer& getSceneStars() { return sceneStars; }
	public:
		Star();
		~Star();
	public:
		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) const;
		glm::vec3 getLightDirection() const;
		void setXform(Transform& xform) { this->xform = xform; }
	protected:
		virtual void postConstruct() override;
	private: //statics
		static sp<SphereMeshTextured> starMesh;
		static uint32_t starInstanceCount;
		static sp<Shader> starShader;
		static StarContainer sceneStars;
	private: //instance variables
		Transform xform;
		glm::vec3 sdr_color = glm::vec3(1.0, 0.886, 0.533); //default is color of sun as seen from earth; which isn't a correct color in space, but I like it.
		float hdr_intensity = 1000.0f;
		bool bUseHDR = false;
		bool bForceCentered = true;

	};

}
