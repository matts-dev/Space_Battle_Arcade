#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include<cstdint>
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

			LEFT_PERIOD			= 1 << 20,
			MIDDLE_PERIOD		= 1 << 21,
			RIGHT_PERIOD		= 1 << 22
		};
	}

	sp<class Shader> getDefaultGlyphShader_uniformBased();

	class DigitalClockGlyph : public GPUResource
	{
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
