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
#include "../../../../Rendering/SAGPUResource.h"
#include <vector>
#include "../../../../Tools/DataStructures/SATransform.h"

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

			//make sure max is correctly defined!
			MAX						= 1 << 29
		};
	}

	sp<class Shader> getDefaultGlyphShader_uniformBased();
	sp<class Shader> getDefaultGlyphShader_instanceBased();

	namespace DCFont
	{
		constexpr size_t NumPossibleValuesInChar = 256;

		struct BatchData
		{
			size_t attribBytesBuffered = 0;
			size_t numBatchesRendered = 0;
			static const size_t MAX_BUFFERABLE_BYTES = 5000000;
		};
	}

	/** A renderer for an single glyph (ie letter). This should be a shared resource */
	class DigitalClockGlyph : public GPUResource
	{
		using Parent = GPUResource;
	public:
		static const std::array<int32_t, DCFont::NumPossibleValuesInChar>& getCharToBitvectorMap();
		static constexpr float GLYPH_WIDTH = 1.0f; 
		static constexpr float GLYPH_HEIGHT = 1.0f; 
		static constexpr float BETWEEN_GLYPH_SPACE = 0.2f; //the unscaled space between two glyphs so that they do not connect and provide visual spacing.
	public:
		void render(Shader& shader);
		bool renderInstanced(Shader& shader, const struct RenderData& rd);
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
		GLuint vbo_instance_models = 0;
		GLuint vbo_instance_parent_pivot = 0;
		GLuint vbo_instance_bitvec = 0;
		GLuint vbo_instance_color = 0;
	};

	/** A renderer for entire strings */
	class DigitalClockFont : public GameEntity
	{
		using Parent = GameEntity;
	public:
		enum class EHorizontalPivot { CENTER, LEFT, RIGHT };
		enum class EVerticalPivot { CENTER, TOP, BOTTOM };
		struct Data;
	protected:
		struct GlyphCalculationCache;
	public:
		DigitalClockFont(const Data& init = {});
		virtual ~DigitalClockFont();
		void render(const struct RenderData& rd);			
		void renderGlyphsAsInstanced(const struct RenderData& rd);	//requires initialization with instanced version of shader

		bool prepareBatchedInstance(const DigitalClockFont& addToBatch, DCFont::BatchData& batchData);
		void renderBatched(const struct RenderData& rd, DCFont::BatchData& batchData);
	public:
		void setText(const std::string& newText);
		const Transform& getXform() const { return xform; }
		void setXform(const Transform& newXform);
		float getWidth() const;
		float getHeight() const;
	protected:
		virtual void postConstruct() override;
		void rebuildDataCache();
		virtual void onGlyphCacheRebuilt(const GlyphCalculationCache& data) {};
		virtual void preIndividualGlyphRender(size_t idx, Shader& shader) {} /** Only called on non-instanced/batched glyphs. Allows cstom shader parameters*/
	private: //statics
		static sp<DigitalClockGlyph> sharedGlyph;
		static uint64_t numFontInstances;
	public: //public so this can be filled out externally and passed to ctor; d3d style
		struct Data
		{
			sp<Shader> shader;
			EHorizontalPivot pivotHorizontal = EHorizontalPivot::CENTER;
			EVerticalPivot pivotVertical = EVerticalPivot::CENTER;
			std::string text = "";
			glm::vec4 fontColor = glm::vec4(1.f);
		};
	protected:
		struct GlyphCalculationCache
		{
			std::vector<glm::mat4> glyphModelMatrices;
			std::vector<int> glyphBitVectors;
			std::vector<glm::vec4> glyphColors;
			glm::mat4 paragraphPivotMat{ 1.f };
			glm::mat4 paragraphModelMat{ 1.f };
			size_t bufferedChars;
		};
		float AdditionalGlyphSpacingFactor = 1.0f;
		glm::vec2 paragraphSize; //width and height of the paragraph
	private:
		Data data;
		GlyphCalculationCache cache;
		Transform xform; //needs to be strongly encapsulated so we don't recalculate most of cache.
	};


}
