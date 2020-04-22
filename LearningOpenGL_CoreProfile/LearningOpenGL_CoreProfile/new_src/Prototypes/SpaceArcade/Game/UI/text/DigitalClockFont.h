#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include<cstdint>
#include<cmath>

#include <glad/glad.h>
#include <array>
#include <GLFW/glfw3.h>
#include "../../../Rendering/SAGPUResource.h"
#include <vector>

namespace SA
{
	/** Digital clock bars */
	namespace DCBars
	{
		enum Val
		{
			//horizontal bars
			TOP_RIGHT			= 1,
			TOP_LEFT			= 1 << 1,
			MIDDLE_RIGHT		= 1 << 2,
			MIDDLE_LEFT			= 1 << 3,
			BOTTOM_RIGHT		= 1 << 4,
			BOTTOM_LEFT			= 1 << 5,

			//vertical bars
			LEFT_TOP			= 1 << 6,
			LEFT_BOTTOM			= 1 << 7,
			MIDDLE_TOP			= 1 << 8,
			MIDDLE_BOTTOM		= 1 << 9,
			RIGHT_TOP			= 1 << 10,
			RIGHT_BOTTOM		= 1 << 11,

			FORWARD_SLASH_TL	= 1 << 12,
			FORWARD_SLASH_TR	= 1 << 13,
			FORWARD_SLASH_BL	= 1 << 14,
			FORWARD_SLASH_BR	= 1 << 15,

			BACK_SLASH_TL		= 1 << 16,
			BACK_SLASH_TR		= 1 << 17,
			BACK_SLASH_BL		= 1 << 18,
			BACK_SLASH_BR		= 1 << 19,

			BOTTOM_LEFT_PERIOD		= 1 << 20,
			BOTTOM_MIDDLE_PERIOD	= 1 << 21,
			BOTTOM_RIGHT_PERIOD		= 1 << 22,
			MIDDLE_LEFT_PERIOD		= 1 << 23,
			MIDDLE_MIDDLE_PERIOD	= 1 << 24,
			MIDDLE_RIGHT_PERIOD		= 1 << 25,
			TOP_LEFT_PERIOD			= 1 << 26,
			TOP_MIDDLE_PERIOD		= 1 << 27,
			TOP_RIGHT_PERIOD		= 1 << 28,

		};
	}

	sp<class Shader> getDefaultGlyphShader_uniformBased();

	namespace DCFont
	{
		constexpr size_t NumPossibleValuesInChar = 256;
	}

	class DigitalClockGlyph : public GPUResource
	{
	public:
		static const std::array<int32_t, DCFont::NumPossibleValuesInChar>& getCharToBitvectorMap();
		static constexpr float GLYPH_WIDTH = 1.0f; 
		static constexpr float GLYPH_HEIGHT = 1.0f; 
		static constexpr float BETWEEN_GLYPH_SPACE = 0.2f; //the unscaled space between two glyphs so that they do not connect and provide visual spacing.
	public:
		void render(struct GameUIRenderData& renderData, Shader& shader);
		virtual ~DigitalClockGlyph();
	protected:
		virtual void postConstruct() override;
		virtual void onReleaseGPUResources();
		virtual void onAcquireGPUResources();
	private:
		std::vector<glm::vec4> vertex_positions;
		std::vector<glm::vec2> vertex_uvs;
		std::vector<int> vertex_bits;
	private: //gpu resources
		GLuint vao = 0;
		GLuint vbo_pos = 0;
		GLuint vbo_uvs = 0;
		GLuint vbo_bits = 0;
	};

}
