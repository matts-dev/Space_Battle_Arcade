#include "DigitalClockFont.h"
#include "../../../Rendering/OpenGLHelpers.h"
#include "../../../Rendering/SAShader.h"
#include "../../../Rendering/RenderData.h"
#include "../../../GameFramework/SALog.h"

namespace SA
{
	////////////////////////////////////////////////////////
	// statics
	////////////////////////////////////////////////////////
	const std::array<int32_t, DCFont::NumPossibleValuesInChar>& DigitalClockGlyph::getCharToBitvectorMap()
	{
		static std::array<int32_t, DCFont::NumPossibleValuesInChar> map;
		static int oneTimeInit = [&]()
		{
			std::memset(map.data(), 0xFFFFFFFF, map.size() * sizeof(int)); //have unset characters show everything for easy debugging

			const int32_t left = DCBars::LEFT_TOP | DCBars::LEFT_BOTTOM;
			const int32_t leftBar = left | DCBars::MIDDLE_LEFT_PERIOD;
			const int32_t leftBarFull = leftBar | DCBars::BOTTOM_LEFT_PERIOD | DCBars::TOP_LEFT_PERIOD;
			const int32_t right = DCBars::RIGHT_TOP | DCBars::RIGHT_BOTTOM;
			const int32_t rightBar = right | DCBars::MIDDLE_RIGHT_PERIOD;
			const int32_t rightBarFull = rightBar | DCBars::BOTTOM_RIGHT_PERIOD | DCBars::TOP_RIGHT_PERIOD;
			const int32_t top = DCBars::TOP_LEFT | DCBars::TOP_RIGHT;
			const int32_t topBar = top | DCBars::TOP_MIDDLE_PERIOD;
			const int32_t topBarFull = topBar | DCBars::TOP_LEFT_PERIOD | DCBars::TOP_RIGHT_PERIOD;
			const int32_t middle = DCBars::MIDDLE_LEFT | DCBars::MIDDLE_RIGHT;
			const int32_t middleBar = middle | DCBars::MIDDLE_MIDDLE_PERIOD;
			const int32_t middleBarFull = middleBar | DCBars::MIDDLE_RIGHT_PERIOD | DCBars::MIDDLE_LEFT_PERIOD;
			const int32_t bottom = DCBars::BOTTOM_LEFT | DCBars::BOTTOM_RIGHT;
			const int32_t bottomBar = bottom | DCBars::BOTTOM_MIDDLE_PERIOD;
			const int32_t bottomBarFull = bottomBar | DCBars::BOTTOM_LEFT_PERIOD | DCBars::BOTTOM_RIGHT_PERIOD;
			const int32_t circle = left | right | top | bottom;
			const int32_t circleBar = circle | DCBars::TOP_MIDDLE_PERIOD | DCBars::BOTTOM_MIDDLE_PERIOD | DCBars::MIDDLE_LEFT_PERIOD | DCBars::MIDDLE_RIGHT_PERIOD;

			const int32_t vertMiddle = DCBars::MIDDLE_TOP| DCBars::MIDDLE_BOTTOM;
			const int32_t vertMiddleBar = vertMiddle | DCBars::MIDDLE_MIDDLE_PERIOD;
			const int32_t vertMiddleBarFull = vertMiddleBar | DCBars::TOP_MIDDLE_PERIOD| DCBars::BOTTOM_MIDDLE_PERIOD;

			const int32_t forwardSlash = DCBars::FORWARD_SLASH_BL | DCBars::FORWARD_SLASH_TR;
			const int32_t backSlash = DCBars::BACK_SLASH_TL| DCBars::BACK_SLASH_BR;


			map[' '] = 0;
			map['a'] = map['A'] = left | right | topBar | middleBarFull | DCBars::BOTTOM_LEFT_PERIOD | DCBars::BOTTOM_RIGHT_PERIOD;
			map['b'] = map['B'] = (circleBar | middleBar | DCBars::BOTTOM_LEFT_PERIOD | DCBars::TOP_LEFT_PERIOD) - DCBars::MIDDLE_RIGHT_PERIOD;
			map['c'] = map['C'] = leftBarFull | topBar | bottomBar;
			map['d'] = map['D'] = leftBarFull | topBar | bottomBar | rightBar;
			map['e'] = map['E'] = leftBarFull | topBar | middleBar | bottomBar | DCBars::TOP_RIGHT_PERIOD | DCBars::BOTTOM_RIGHT_PERIOD;
			map['f'] = map['F'] = leftBarFull | topBar | middleBar | DCBars::TOP_RIGHT_PERIOD;
			map['g'] = map['G'] = DCBars::MIDDLE_RIGHT | DCBars::RIGHT_BOTTOM | DCBars::BOTTOM_RIGHT_PERIOD |bottomBar | leftBar | topBar | DCBars::TOP_LEFT_PERIOD;
			map['h'] = map['H'] = middleBar | leftBarFull | rightBarFull;
			map['i'] = map['I'] = vertMiddleBarFull | topBar | bottomBar;
			map['j'] = map['J'] = rightBar | DCBars::TOP_RIGHT | bottomBar| DCBars::BOTTOM_LEFT;
			map['k'] = map['K'] = leftBarFull | DCBars::MIDDLE_LEFT | DCBars::FORWARD_SLASH_TR | DCBars::BACK_SLASH_BR| DCBars::MIDDLE_MIDDLE_PERIOD | DCBars::BOTTOM_RIGHT_PERIOD | DCBars::TOP_RIGHT_PERIOD;
			map['l'] = map['L'] =  leftBarFull | bottomBar;
			map['m'] = map['M'] =  leftBarFull | rightBarFull | DCBars::BACK_SLASH_TL | DCBars::FORWARD_SLASH_TR | DCBars::MIDDLE_MIDDLE_PERIOD/*| DCBars::MIDDLE_BOTTOM*/;
			map['n'] = map['N'] =  leftBarFull | DCBars::BACK_SLASH_TL | DCBars::MIDDLE_MIDDLE_PERIOD | DCBars::BACK_SLASH_BR | rightBarFull ;
			map['o'] = map['O'] =  circleBar;
			map['p'] = map['P'] =  leftBarFull | topBar | middleBar | DCBars::RIGHT_TOP;
			map['q'] = map['Q'] =  circle | DCBars::BACK_SLASH_BR | DCBars::BOTTOM_RIGHT_PERIOD;
			map['r'] = map['R'] = leftBarFull | middleBar | topBar | DCBars::RIGHT_TOP|DCBars::BACK_SLASH_BR | DCBars::BOTTOM_RIGHT_PERIOD;
			map['s'] = map['S'] =  topBar | middleBar | bottomBar | DCBars::LEFT_TOP | DCBars::RIGHT_BOTTOM;
			map['t'] = map['T'] =  vertMiddleBarFull | topBarFull;
			map['u'] = map['U'] =  leftBar | bottomBar | rightBar;
			map['v'] = map['V'] = DCBars::BACK_SLASH_BL | DCBars::FORWARD_SLASH_BR | DCBars::RIGHT_TOP| DCBars::LEFT_TOP|DCBars::BOTTOM_MIDDLE_PERIOD | DCBars::MIDDLE_RIGHT_PERIOD| DCBars::MIDDLE_LEFT_PERIOD;
			map['w'] = map['W'] = leftBarFull | rightBarFull | DCBars::BACK_SLASH_BR | DCBars::FORWARD_SLASH_BL| DCBars::MIDDLE_MIDDLE_PERIOD/*|DCBars::MIDDLE_TOP*/;
			map['x'] = map['X'] = DCBars::BACK_SLASH_TL | DCBars::BACK_SLASH_BR | DCBars::FORWARD_SLASH_TR | DCBars::FORWARD_SLASH_BL| DCBars::MIDDLE_MIDDLE_PERIOD;
			map['y'] = map['Y'] = DCBars::BACK_SLASH_TL | DCBars::FORWARD_SLASH_TR | DCBars::MIDDLE_BOTTOM| DCBars::MIDDLE_MIDDLE_PERIOD;
			map['z'] = map['Z'] =  topBar | DCBars::FORWARD_SLASH_TR | DCBars::FORWARD_SLASH_BL | bottomBar| DCBars::MIDDLE_MIDDLE_PERIOD;

			map['!'] = DCBars::MIDDLE_TOP | DCBars::BOTTOM_MIDDLE_PERIOD;
			map['@'] = topBarFull | bottomBarFull | leftBarFull|rightBarFull|DCBars::BACK_SLASH_BL |DCBars::BACK_SLASH_TR|DCBars::FORWARD_SLASH_BR|DCBars::FORWARD_SLASH_TL;
			map['#'] = /*topBarFull | middleBarFull |*/ DCBars::BACK_SLASH_BL | DCBars::BACK_SLASH_BR | DCBars::BACK_SLASH_TL| DCBars::BACK_SLASH_TR
						|DCBars::FORWARD_SLASH_BL | DCBars::FORWARD_SLASH_BR | DCBars::FORWARD_SLASH_TL | DCBars::FORWARD_SLASH_TR;
			map['$'] = topBar | middleBar | bottomBar | DCBars::LEFT_TOP| DCBars::RIGHT_BOTTOM| vertMiddleBarFull;
			map['%'] = forwardSlash | DCBars::FORWARD_SLASH_TL | DCBars::FORWARD_SLASH_BR | DCBars::BACK_SLASH_TL | DCBars::BACK_SLASH_BR;
			map['^'] = DCBars::FORWARD_SLASH_TL | DCBars::BACK_SLASH_TR|DCBars::TOP_MIDDLE_PERIOD;
			map['&'] = backSlash | topBar| DCBars::RIGHT_TOP |DCBars::MIDDLE_RIGHT|DCBars::FORWARD_SLASH_BL|DCBars::BOTTOM_LEFT|DCBars::FORWARD_SLASH_BL;
			map['*'] = DCBars::BACK_SLASH_TL|DCBars::FORWARD_SLASH_TL;
			map['('] = leftBar|DCBars::BOTTOM_LEFT|DCBars::TOP_LEFT;
			map[')'] = rightBar|DCBars::BOTTOM_RIGHT|DCBars::TOP_RIGHT;
			map['-'] = middleBar;
			map['_'] = bottomBarFull;
			map['+'] = middleBar|vertMiddleBar;
			map['='] = middleBar|topBar;

			map['\''] = DCBars::TOP_MIDDLE_PERIOD;
			map['"'] = DCBars::TOP_MIDDLE_PERIOD | DCBars::TOP_RIGHT_PERIOD;
			map[':'] = DCBars::MIDDLE_MIDDLE_PERIOD | DCBars::BOTTOM_MIDDLE_PERIOD;
			map[';'] = DCBars::MIDDLE_MIDDLE_PERIOD | DCBars::BOTTOM_MIDDLE_PERIOD|DCBars::BOTTOM_LEFT;
			map['.'] = DCBars::BOTTOM_MIDDLE_PERIOD;
			map['>'] = DCBars::BACK_SLASH_TL | DCBars::FORWARD_SLASH_BL | DCBars::MIDDLE_RIGHT_PERIOD;
			map['<'] = DCBars::BACK_SLASH_BR | DCBars::FORWARD_SLASH_TR | DCBars::MIDDLE_LEFT_PERIOD;
			map[','] = DCBars::BOTTOM_MIDDLE_PERIOD | DCBars::BOTTOM_LEFT;
			map['/'] = forwardSlash |DCBars::MIDDLE_MIDDLE_PERIOD;
			map['\\'] = backSlash | DCBars::MIDDLE_MIDDLE_PERIOD;
			map['?'] = topBar|DCBars::RIGHT_TOP|DCBars::MIDDLE_RIGHT|DCBars::MIDDLE_BOTTOM;
			map['|'] = middleBarFull;
			map['~'] = DCBars::MIDDLE_LEFT|DCBars::MIDDLE_TOP|DCBars::MIDDLE_RIGHT;
			map['`'] = DCBars::TOP_LEFT_PERIOD;

			map['['] = leftBar | DCBars::BOTTOM_LEFT | DCBars::TOP_LEFT | DCBars::TOP_LEFT_PERIOD | DCBars::BOTTOM_LEFT_PERIOD;
			map[']'] = rightBar | DCBars::BOTTOM_RIGHT | DCBars::TOP_RIGHT | DCBars::TOP_RIGHT_PERIOD | DCBars::BOTTOM_RIGHT_PERIOD;
			map['{'] = vertMiddleBar | DCBars::MIDDLE_LEFT|DCBars::TOP_RIGHT|DCBars::BOTTOM_RIGHT;
			map['}'] = vertMiddleBar | DCBars::MIDDLE_RIGHT | DCBars::TOP_LEFT | DCBars::BOTTOM_LEFT;

			map['1'] = right;
			map['2'] = topBar|middleBar|bottomBar|DCBars::LEFT_BOTTOM|DCBars::RIGHT_TOP;
			map['3'] = right|bottomBar|middleBar|topBar;
			map['4'] = DCBars::LEFT_TOP|middleBar|right|DCBars::TOP_LEFT_PERIOD|DCBars::TOP_RIGHT_PERIOD;
			map['5'] = topBar|middleBar|bottomBar|DCBars::LEFT_TOP|DCBars::RIGHT_BOTTOM;
			map['6'] = bottomBar | middleBar | topBar | DCBars::RIGHT_BOTTOM | leftBar;
			map['7'] = right|topBar;
			map['8'] = topBar|middleBar|bottomBar|left|right;
			map['9'] = right|topBar|middleBar|DCBars::LEFT_TOP;
			map['0'] = circle;
			
			return 0;
		}();
		return map;
	}

	//not best solution to make these static, but works for now. Perhaps better is to have these stored in some render system for memory clean up and unit testing efficency
	namespace InstanceBuffers
	{
		static std::vector<glm::mat4> modelMats;
		static std::vector<glm::mat4> parentPivotMats;
		static std::vector<glm::vec4> glyphColors;
		static std::vector<int32_t> bitVectors;
		static const int onetimeReserveCall = []()
		{
			size_t reserveSize = 1000;
			modelMats.reserve(reserveSize);
			parentPivotMats.reserve(reserveSize);
			glyphColors.reserve(reserveSize);
			bitVectors.reserve(reserveSize);
			return 0;
		}();
	}

	//#TODO expose these shaders publicly
	static const char*const  DigitalClockShader_uniformDrive_vs = R"(
		#version 330 core
		layout (location = 0) in vec4 vertPos;				
		layout (location = 1) in vec2 vertUVs;				
		layout (location = 2) in int vertBitVec;				
				
		out vec2 uvs;
		out vec4 color;

		uniform mat4 glyphModel = mat4(1.f);
		uniform mat4 projection_view = mat4(1.f);
		uniform mat4 pivotMat = mat4(1.f);
		uniform mat4 parentModel = mat4(1.f);
		uniform vec4 uColor = vec4(1.f);

		uniform int bitVec = 0xFFFFFFFF; //replace this with an instanced attribute

		void main()
		{
			//either create identity matrix or zero matrix to filter out digital clock segments
			mat4 filterMatrix =	mat4((bitVec & vertBitVec) > 0 ? 1.f : 0.f);		//mat4(1.f) == identity matrix

			gl_Position = projection_view * parentModel * pivotMat * glyphModel * filterMatrix * vertPos;
			uvs = vertUVs;
			color = uColor;
		}
	)";
	static const char*const  DigitalClockShader_instanced_vs = R"(
		#version 330 core
		layout (location = 0) in vec4 vertPos;				
		layout (location = 1) in vec2 vertUVs;				
		layout (location = 2) in int vertBitVec;	//basically defineds what bitvector value this vertex should render under			
		layout (location = 3) in mat4 glyphModel;
		  //layout (location = 4) in mat4 glyphModel;
		  //layout (location = 5) in mat4 glyphModel;
		  //layout (location = 6) in mat4 glyphModel;
		layout (location = 7) in mat4 parent_pivot;
		  //layout (location = 8) in mat4 parent_pivot;
		  //layout (location = 9) in mat4 parent_pivot;
		  //layout (location = 10) in mat4 parent_pivot;
		layout (location = 11) in vec4 glyphColor;
		layout (location = 12) in int bitVec;
		
		out vec2 uvs;
		out vec4 color;

		uniform mat4 projection_view = mat4(1.f);

		void main()
		{
			//either create identity matrix or zero matrix to filter out digital clock segments
			mat4 filterMatrix =	mat4((bitVec & vertBitVec) > 0 ? 1.f : 0.f);		//mat4(1.f) == identity matrix

			gl_Position = projection_view * parent_pivot * glyphModel * filterMatrix * vertPos;

			uvs = vertUVs;
			color = glyphColor;
		}
	)";
	static const char*const DigitalClockShader_instanced_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		in vec4 color;

		void main()
		{
			fragmentColor = color;
		}
	)";
	sp<Shader> getDefaultGlyphShader_uniformBased()
	{
		return new_sp<Shader>(DigitalClockShader_uniformDrive_vs, DigitalClockShader_instanced_fs, false);
	}
	sp<Shader> getDefaultGlyphShader_instanceBased()
	{
		return new_sp<Shader>(DigitalClockShader_instanced_vs, DigitalClockShader_instanced_fs, false);
	}


	void DigitalClockGlyph::render(Shader& shader)
	{
		if (vao)
		{
			shader.use();

			ec(glBindVertexArray(vao));
			ec(glDrawArrays(GL_TRIANGLES, 0, vertex_positions.size()));
		}
	}

	bool DigitalClockGlyph::renderInstanced(Shader& shader, const struct RenderData& rd)
	{
		bool bRendered = false;

		if (hasAcquiredResources() && vao)
		{
			ec(glBindVertexArray(vao));

			////////////////////////////////////////////////////////
			// glyph model matrices
			////////////////////////////////////////////////////////
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_models));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * InstanceBuffers::modelMats.size(), &InstanceBuffers::modelMats[0], GL_DYNAMIC_DRAW)); 
			ec(glEnableVertexAttribArray(3));
			ec(glEnableVertexAttribArray(4));
			ec(glEnableVertexAttribArray(5));
			ec(glEnableVertexAttribArray(6));

			ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec4), reinterpret_cast<void*>(0 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec4), reinterpret_cast<void*>(1 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec4), reinterpret_cast<void*>(2 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec4), reinterpret_cast<void*>(3* sizeof(glm::vec4))));

			ec(glVertexAttribDivisor(3, 1));
			ec(glVertexAttribDivisor(4, 1));
			ec(glVertexAttribDivisor(5, 1));
			ec(glVertexAttribDivisor(6, 1));

			////////////////////////////////////////////////////////
			// parent and pivot
			////////////////////////////////////////////////////////
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_parent_pivot));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * InstanceBuffers::parentPivotMats.size(), &InstanceBuffers::parentPivotMats[0], GL_DYNAMIC_DRAW));
			ec(glEnableVertexAttribArray(7));
			ec(glEnableVertexAttribArray(8));
			ec(glEnableVertexAttribArray(9));
			ec(glEnableVertexAttribArray(10));

			ec(glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(0 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(1 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(2 * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(3 * sizeof(glm::vec4))));

			ec(glVertexAttribDivisor(7, 1));
			ec(glVertexAttribDivisor(8, 1));
			ec(glVertexAttribDivisor(9, 1));
			ec(glVertexAttribDivisor(10, 1));

			////////////////////////////////////////////////////////
			// colors
			////////////////////////////////////////////////////////
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_color));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * InstanceBuffers::glyphColors.size(), &InstanceBuffers::glyphColors[0], GL_DYNAMIC_DRAW));
			ec(glEnableVertexAttribArray(11));
			ec(glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), reinterpret_cast<void*>(0)));
			ec(glVertexAttribDivisor(11, 1));

			////////////////////////////////////////////////////////
			// bitvectors
			////////////////////////////////////////////////////////
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_bitvec));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(int32_t) * InstanceBuffers::bitVectors.size(), &InstanceBuffers::bitVectors[0], GL_DYNAMIC_DRAW));
			ec(glEnableVertexAttribArray(12));
			ec(glVertexAttribIPointer(12, 1, GL_INT, sizeof(int32_t), reinterpret_cast<void*>(0)));
			ec(glVertexAttribDivisor(12, 1));


			/////////////////////////////////////////////////////////////////////////////////////
			// RENDER WITH BUFFERED DATA
			/////////////////////////////////////////////////////////////////////////////////////
			shader.use();
			ec(glBindVertexArray(vao));
			ec(glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_positions.size(), GLsizei(InstanceBuffers::bitVectors.size())));

			//clear out the data to prevent memory leak, these will fill up as instances are prepared
			InstanceBuffers::bitVectors.clear();
			InstanceBuffers::modelMats.clear();
			InstanceBuffers::parentPivotMats.clear();
			InstanceBuffers::glyphColors.clear();

			bRendered = true;
		}

		return bRendered;
	}

	DigitalClockGlyph::~DigitalClockGlyph()
	{
		onReleaseGPUResources();

	}

	void DigitalClockGlyph::postConstruct()
	{	
		GPUResource::postConstruct();

		using namespace glm;

		vec4 baseSquare[] = 
		{
			vec4(-0.5f, -0.5f, 0, 1.f),
			vec4(0.5f, -0.5f, 0, 1.f),
			vec4(-0.5f, 0.5f, 0, 1.f),

			vec4(0.5f, -0.5f, 0, 1.f),
			vec4(0.5f, 0.5f, 0, 1.f),
			vec4(-0.5f, 0.5f, 0, 1.f)
		};

		vec2 baseUV[] = {
			vec2(0,0),
			vec2(1,0),
			vec2(0,1),

			vec2(1,0),
			vec2(1,1),
			vec2(0,1)
		};

		auto transformAndAddVerts = [&](const glm::mat4& transform, int bit) 
		{
			for(size_t vertIdx = 0; vertIdx < 6; ++vertIdx)
			{
				vertex_positions.push_back(transform* baseSquare[vertIdx]);
				vertex_uvs.push_back(baseUV[vertIdx]);
				vertex_bits.push_back(bit);
			}
		};


		//mat4 gsd_m = glm::scale(mat4(1.f), vec3(0.1f, 0.1f, 0.1f)); //global scale down
		mat4 gsd_m = glm::scale(mat4(1.f), vec3(1.f)); //global scale down
		mat4 zero_m{ 0.f };

		//useful positions to composite together
		const float boxRadius = 0.5f; //keep this at specific value so a glyph width/height is 1; this will make spacing/scaling math much easier!
		vec3 topRowPos = vec3(0, boxRadius, 0);
		vec3 rightColPos = vec3(boxRadius, 0, 0);
		vec3 bottomRowPos = -topRowPos;
		vec3 leftColPos = -rightColPos;

		mat4 rotPos45 = glm::toMat4(glm::angleAxis(radians<float>(45), vec3(0, 0, 1)));
		mat4 rotNeg45 = glm::toMat4(glm::angleAxis(radians<float>(-45), vec3(0, 0, 1)));

		mat4 translate_m{ 1.f };

		auto trans = [](glm::vec3 pos) {
			return glm::translate(mat4(1.f), pos);
		};
		auto scale = [](glm::vec2 scaleVec){
			return glm::scale(mat4(1.f), vec3(scaleVec,1));
		};
		translate_m = glm::translate(mat4(1.f), topRowPos);
		float boxLength = boxRadius * 0.75f;
		mat4 horScale_m = scale({ boxLength			   ,boxRadius - boxLength });
		mat4 verScale_m = scale({ boxRadius - boxLength,			boxLength});

		////////////////////////////////////////////////////////
		// horizontal
		////////////////////////////////////////////////////////
		translate_m = trans(topRowPos + (rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::TOP_RIGHT);

		translate_m = trans(topRowPos + (leftColPos/ 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::TOP_LEFT);

		translate_m = trans((rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::MIDDLE_RIGHT);

		translate_m = trans((leftColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::MIDDLE_LEFT);

		translate_m = trans(bottomRowPos + (rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::BOTTOM_RIGHT);

		translate_m = trans(bottomRowPos + (leftColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * horScale_m, DCBars::BOTTOM_LEFT);

		////////////////////////////////////////////////////////
		// vertical
		////////////////////////////////////////////////////////
		translate_m = trans((topRowPos/2.f) + leftColPos);
		transformAndAddVerts(gsd_m * translate_m * verScale_m , DCBars::LEFT_TOP);

		translate_m = trans((bottomRowPos / 2.f) + leftColPos);
		transformAndAddVerts(gsd_m * translate_m * verScale_m, DCBars::LEFT_BOTTOM);

		translate_m = trans((topRowPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * verScale_m, DCBars::MIDDLE_TOP);
		
		translate_m = trans((bottomRowPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * verScale_m, DCBars::MIDDLE_BOTTOM);

		translate_m = trans((topRowPos / 2.f) + rightColPos);
		transformAndAddVerts(gsd_m * translate_m * verScale_m, DCBars::RIGHT_TOP);

		translate_m = trans((bottomRowPos / 2.f) + rightColPos);
		transformAndAddVerts(gsd_m * translate_m * verScale_m, DCBars::RIGHT_BOTTOM);

		
		////////////////////////////////////////////////////////
		// forward slash
		////////////////////////////////////////////////////////
		translate_m = trans((topRowPos/ 2.f) + (leftColPos/ 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotPos45 * horScale_m, DCBars::FORWARD_SLASH_TL);

		translate_m = trans((topRowPos / 2.f) + (rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotPos45 * horScale_m, DCBars::FORWARD_SLASH_TR);

		translate_m = trans((bottomRowPos / 2.f) + (leftColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotPos45 * horScale_m, DCBars::FORWARD_SLASH_BL);

		translate_m = trans((bottomRowPos / 2.f) + (rightColPos /2.f));
		transformAndAddVerts(gsd_m * translate_m * rotPos45 * horScale_m, DCBars::FORWARD_SLASH_BR);

		////////////////////////////////////////////////////////
		// back slash
		////////////////////////////////////////////////////////
		translate_m = trans((topRowPos / 2.f) + (leftColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotNeg45 * horScale_m, DCBars::BACK_SLASH_TL);

		translate_m = trans((topRowPos / 2.f) + (rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotNeg45 * horScale_m, DCBars::BACK_SLASH_TR);

		translate_m = trans((bottomRowPos / 2.f) + (leftColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotNeg45 * horScale_m, DCBars::BACK_SLASH_BL);

		translate_m = trans((bottomRowPos / 2.f) + (rightColPos / 2.f));
		transformAndAddVerts(gsd_m * translate_m * rotNeg45 * horScale_m, DCBars::BACK_SLASH_BR);

		////////////////////////////////////////////////////////
		// special
		////////////////////////////////////////////////////////
		mat4 specialScaleDown = glm::scale(mat4(1.f), vec3(boxRadius - boxLength));
		translate_m = trans(bottomRowPos + leftColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::BOTTOM_LEFT_PERIOD);

		translate_m = trans(bottomRowPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::BOTTOM_MIDDLE_PERIOD);

		translate_m = trans(bottomRowPos + rightColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::BOTTOM_RIGHT_PERIOD);

		translate_m = trans(leftColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::MIDDLE_LEFT_PERIOD);

		translate_m = trans(vec3(0,0,0));
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::MIDDLE_MIDDLE_PERIOD);

		translate_m = trans(rightColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::MIDDLE_RIGHT_PERIOD);

		translate_m = trans(topRowPos+ leftColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::TOP_LEFT_PERIOD);

		translate_m = trans(topRowPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::TOP_MIDDLE_PERIOD);

		translate_m = trans(topRowPos + rightColPos);
		transformAndAddVerts(gsd_m * translate_m * specialScaleDown, DCBars::TOP_RIGHT_PERIOD);

	}

	
	void DigitalClockGlyph::onAcquireGPUResources()
	{
		if (!vao)
		{
			ec(glGenVertexArrays(1, &vao));
			ec(glBindVertexArray(vao));

			//positions
			ec(glGenBuffers(1, &vbo_pos));
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_pos));
			ec(glBufferData(GL_ARRAY_BUFFER, vertex_positions.size() * sizeof(decltype(vertex_positions)::value_type), vertex_positions.data(), GL_STATIC_DRAW));
			ec(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<GLvoid*>(0)));
			glEnableVertexAttribArray(0);
			
			//uvs
			ec(glGenBuffers(1, &vbo_uvs));
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs));
			ec(glBufferData(GL_ARRAY_BUFFER, vertex_uvs.size() * sizeof(decltype(vertex_uvs)::value_type), vertex_uvs.data(), GL_STATIC_DRAW));
			ec(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<GLvoid*>(0)));
			ec(glEnableVertexAttribArray(1));

			//bits
			ec(glGenBuffers(1, &vbo_bits));
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_bits));
			ec(glBufferData(GL_ARRAY_BUFFER, vertex_bits.size() * sizeof(decltype(vertex_bits)::value_type), vertex_bits.data(), GL_STATIC_DRAW));
			ec(glVertexAttribIPointer(2, 1, GL_INT, sizeof(decltype(vertex_bits)::value_type), reinterpret_cast<void*>(0)));
			ec(glEnableVertexAttribArray(2));

			//generate instance data buffers for optimized instance rendering
			ec(glGenBuffers(1, &vbo_instance_models));
			ec(glGenBuffers(1, &vbo_instance_bitvec));
			ec(glGenBuffers(1, &vbo_instance_color));
			ec(glGenBuffers(1, &vbo_instance_parent_pivot));

			ec(glBindVertexArray(0));
		}
	}

	void DigitalClockGlyph::onReleaseGPUResources()
	{
		if (vao)
		{
			ec(glDeleteVertexArrays(1, &vao));
			ec(glDeleteBuffers(1, &vbo_pos));
			ec(glDeleteBuffers(1, &vbo_uvs));
			ec(glDeleteBuffers(1, &vbo_bits));
			ec(glDeleteBuffers(1, &vbo_instance_models));
			ec(glDeleteBuffers(1, &vbo_instance_parent_pivot));
			ec(glDeleteBuffers(1, &vbo_instance_bitvec));
			ec(glDeleteBuffers(1, &vbo_instance_color));
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Digital clock font
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*static*/sp<SA::DigitalClockGlyph> DigitalClockFont::sharedGlyph = nullptr;
	/*static*/uint64_t DigitalClockFont::numFontInstances = 0;
	DigitalClockFont::DigitalClockFont(const Data& init /*= {}*/) : data(init)
	{
		if (numFontInstances == 0)
		{
			sharedGlyph = new_sp<DigitalClockGlyph>();
		}
		++numFontInstances;

		if (!data.shader)
		{
			data.shader = getDefaultGlyphShader_uniformBased();
		}
	}

	DigitalClockFont::~DigitalClockFont()
	{
		--numFontInstances;
		if (numFontInstances == 0)
		{
			sharedGlyph = nullptr;
		}
	}

	void DigitalClockFont::render(const RenderData& rd)
	{
		if (cache.glyphModelMatrices.size() == cache.glyphBitVectors.size() && data.shader)
		{
			data.shader->use();
			data.shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(rd.projection_view));
			data.shader->setUniformMatrix4fv("pivotMat", 1, GL_FALSE, glm::value_ptr(cache.paragraphPivotMat));
			data.shader->setUniformMatrix4fv("parentModel", 1, GL_FALSE, glm::value_ptr(cache.paragraphModelMat));

			for (size_t glyphIdx = 0; glyphIdx < cache.glyphBitVectors.size(); ++glyphIdx)
			{
				data.shader->setUniformMatrix4fv("glyphModel", 1, GL_FALSE, glm::value_ptr(cache.glyphModelMatrices[glyphIdx]));
				data.shader->setUniform1i("bitVec", cache.glyphBitVectors[glyphIdx]);
				sharedGlyph->render(*data.shader);
			}
		}
	}

	void DigitalClockFont::renderGlyphsAsInstanced(const struct RenderData& rd)
	{
		DCFont::BatchData batchData;
		if (prepareBatchedInstance(*this, batchData))
		{
			renderBatched(rd, batchData);
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Too many glyphs for a single batch to render with current batch configuration; aborting render.");
		}
	}

	bool DigitalClockFont::prepareBatchedInstance(const DigitalClockFont& addToBatch, DCFont::BatchData& batchData)
	{
		using namespace glm;
		const CachedData& batching = addToBatch.cache;

		size_t numBytesNeeded = 0;
		numBytesNeeded += batching.bufferedChars * sizeof(mat4);	//figure out how data needed for model matrix
		numBytesNeeded += batching.bufferedChars * sizeof(mat4);	//parent_pivot matrix
		numBytesNeeded += batching.bufferedChars * sizeof(vec4);	//glyph colors
		numBytesNeeded += batching.bufferedChars * sizeof(float);	//bit vector

		mat4 parent_pivot = batching.paragraphModelMat * batching.paragraphPivotMat;

		if (numBytesNeeded + batchData.attribBytesBuffered < batchData.MAX_BUFFERABLE_BYTES)
		{
			for (size_t charIdx = 0; charIdx < addToBatch.data.text.length(); ++charIdx)
			{
				InstanceBuffers::modelMats.insert(InstanceBuffers::modelMats.end(), batching.glyphModelMatrices.begin(), batching.glyphModelMatrices.end());
				InstanceBuffers::bitVectors.insert(InstanceBuffers::bitVectors.end(), batching.glyphBitVectors.begin(), batching.glyphBitVectors.end());
				InstanceBuffers::glyphColors.insert(InstanceBuffers::glyphColors.end(), batching.glyphColors.begin(), batching.glyphColors.end());
				InstanceBuffers::parentPivotMats.insert(InstanceBuffers::parentPivotMats.end(), batching.bufferedChars, parent_pivot);
			}

			batchData.attribBytesBuffered += numBytesNeeded;
			return true;
		}
		else
		{
			return false;
		}
	}

	void DigitalClockFont::renderBatched(const struct RenderData& rd, DCFont::BatchData& batchData)
	{
		if (data.shader)
		{
			data.shader->use();
			data.shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(rd.projection_view));
			if (sharedGlyph->renderInstanced(*data.shader, rd))
			{
				batchData.attribBytesBuffered = 0;
				++batchData.numBatchesRendered;
			}
		}
	}

	void DigitalClockFont::postConstruct()
	{
		cache.glyphModelMatrices.reserve(20);
		cache.glyphBitVectors.reserve(20);
		rebuildDataCache();
	}

	void DigitalClockFont::setText(const std::string& newText)
	{
		data.text = newText;
		rebuildDataCache();
	}

	void DigitalClockFont::setXform(const Transform& newXform)
	{
		xform = newXform;
		cache.paragraphModelMat = xform.getModelMatrix();
	}

	void DigitalClockFont::rebuildDataCache()
	{
		using namespace glm;
		using DCG = DigitalClockGlyph;

		const std::array<int32_t, DCFont::NumPossibleValuesInChar>& charToIntMap = DCG::getCharToBitvectorMap();

		cache.glyphBitVectors.clear();
		cache.glyphModelMatrices.clear();
		cache.glyphColors.clear();
		cache.bufferedChars = 0;

		//parse text for rendering; cached for efficiency
		vec2 nextCharPos{ 0.f, 0.f };
		vec2 pgSize{ 0.f, DCG::GLYPH_HEIGHT };	//paragraph size; named this way to visually differeniate it from paragraphEndPoint
		for (size_t charIdx = 0; charIdx < data.text.size(); ++charIdx)
		{
			char letter = data.text[charIdx];
			if (letter == '\n')
			{
				//update paragraph vertical size
				if (charIdx != data.text.size() - 1) //don't bother updating size if this is the last char; size is already configured
				{
					////////////////////////////////////////////////////////
					// set up next glyph position
					////////////////////////////////////////////////////////
					//before we update paragraph size, the next glyph will start at an offset of that size, plus a little spacing.
					nextCharPos.y = -(pgSize.y + DCG::BETWEEN_GLYPH_SPACE); //offset by paragraph size, and add a little spacing
					nextCharPos.x = 0;	//reset horizontal position for this new line

					////////////////////////////////////////////////////////
					// update paragraph size tracking
					////////////////////////////////////////////////////////
					//vec2 paragraphEndPos = nextCharPos;
					//paragraphEndPos.x = 0;	//reset the horizontal tracking since we're starting a new line
					//paragraphEndPos.y -= DCG::GLYPH_HEIGHT + DCG::BETWEEN_GLYPH_SPACE; //may need a vertical space field to use here

					pgSize.y += DCG::BETWEEN_GLYPH_SPACE + DCG::GLYPH_HEIGHT; //in this case the spacing is for previous line, and size is for this line. We start with height of single glyph
				}
			}
			else
			{
				////////////////////////////////////////////////////////
				// set up glyph 
				////////////////////////////////////////////////////////
				vec3 glyphPos = vec3(nextCharPos, 0.f);
				char letter = data.text[charIdx];
				int bitVector = charToIntMap[letter];
				mat4 glyphModel = glm::translate(glm::mat4(1.f), glyphPos);
				cache.glyphColors.push_back(data.fontColor);

				//push the model matrix and a bitvector that defines which portions of digital clock will highlight
				cache.glyphModelMatrices.push_back(glyphModel);
				cache.glyphBitVectors.push_back(bitVector);

				////////////////////////////////////////////////////////
				// set up next glyph position
				////////////////////////////////////////////////////////
				vec2 paragraphEndPos = nextCharPos;
				paragraphEndPos.x += DCG::GLYPH_WIDTH;

				nextCharPos.x = paragraphEndPos.x + DCG::BETWEEN_GLYPH_SPACE;

				////////////////////////////////////////////////////////
				// maintain paragraph size for alignment calculations
				////////////////////////////////////////////////////////
				if (paragraphEndPos.x > pgSize.x)
				{
					pgSize.x = paragraphEndPos.x; //does not include space between glyphs
				}
				++cache.bufferedChars;
			}
		}

		////////////////////////////////////////////////////////
		// pivot alignment
		////////////////////////////////////////////////////////
		vec3 pivotOffset{ DCG::GLYPH_WIDTH/2.f, -DCG::GLYPH_HEIGHT/2.f, 0.f };

		//update horizontal pivot, left assumed ie (0,0) is on the left side
		if (data.pivotHorizontal == EHorizontalPivot::CENTER)
		{
			pivotOffset.x += -pgSize.x / 2;
		}
		else if (data.pivotHorizontal == EHorizontalPivot::RIGHT)
		{
			pivotOffset.x += -pgSize.x;
		}

		//update vertical pivot, top assumed; ie (0,0) is on the top of the text
		if (data.pivotVertical == EVerticalPivot::CENTER)
		{
			pivotOffset.y += pgSize.y / 2;
		}
		else if (data.pivotVertical == EVerticalPivot::BOTTOM)
		{
			pivotOffset.y += pgSize.y;
		}

		cache.paragraphPivotMat = glm::translate(mat4(1.f), pivotOffset);

		cache.paragraphModelMat = xform.getModelMatrix();
	}

}

