#pragma once

#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/DataStructures/SATransform.h"
#include <string>
#include <optional>

namespace SA
{
	class Model3D;
	class Shader;
	class Texture_2D;

	namespace DefaultPlanetTexturesPaths
	{
		const char* const albedo1 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_1.png";
		const char* const albedo2 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_2.png";
		const char* const albedo3 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_3.png";
		const char* const albedo4 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_4.png";
		const char* const albedo5 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_5.png";
		const char* const albedo6 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_6.png";
		const char* const albedo7 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_7.png";
		const char* const albedo8 = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_8.png";
		const char* const albedo_terrain = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_9.png";
		const char* const albedo_sea = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_10.png";
		const char* const albedo_nightlight = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/albedo_11.png";
		const char* const colorChanel = "GameData/mods/SpaceArcade/Assets/Models3D/Planet/color_channel_map.png";
	}
	sp<class Planet> makeRandomPlanet(class RNG& rng);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// An environmental representation of a planet
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Planet : public GameEntity
	{
	public:
		struct Data
		{
			std::optional<std::string> albedo1_filepath;
			std::optional<std::string> albedo2_filepath;
			std::optional<std::string> albedo3_filepath;
			std::optional<std::string> nightCityLightTex_filepath;
			std::optional<std::string> colorMapTex_filepath;
			glm::vec3 orbitAxis = glm::vec3(0, 1, 0);
			float orbitSpeedSec_rad = glm::radians(0.f);
			Transform xform;
		};
		Planet(const Planet::Data& initData) : data(initData) { applySizeCorrections(); };
	public:
		virtual void postConstruct() override;
		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection);
		void setTransform(const Transform& newXform) { data.xform = newXform; }
		Transform getTransform() { return data.xform; }
		void setForceCentered(bool bInForceCentered) { bUseLargeDistanceApproximation = bInForceCentered; }
	private:
		void applySizeCorrections();
	private: //statics
		static sp<Model3D> planetModel;
		static sp<Shader> planetShader;

		sp<Texture_2D> albedo0Tex;
		sp<Texture_2D> albedo1Tex;
		sp<Texture_2D> albedo2Tex;
		sp<Texture_2D> citylightTex;
		sp<Texture_2D> colorMapTex;
	private: //instance data

		Data data;
		bool bUseLargeDistanceApproximation = true;
	};

}
