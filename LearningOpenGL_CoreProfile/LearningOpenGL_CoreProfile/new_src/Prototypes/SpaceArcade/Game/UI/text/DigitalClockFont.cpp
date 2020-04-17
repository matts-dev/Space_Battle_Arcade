#include "DigitalClockFont.h"
#include "../../../Rendering/OpenGLHelpers.h"
#include "../../../Rendering/SAShader.h"

namespace SA
{
	//#TODO expose these shaders publicly
	static const char*const  DigitalClockShader_uniformDrive_vs = R"(
		#version 330 core
		layout (location = 0) in vec4 vertPos;				
		layout (location = 1) in vec2 vertUVs;				
		layout (location = 2) in int vertBitVec;				
				
		out vec2 uvs;

		uniform mat4 model = mat4(1.f);
		uniform mat4 view = mat4(1.f);
		uniform mat4 projection = mat4(1.f);

		uniform int bitVec = 0xFFFFFF; //replace this with an instanced attribute

		void main()
		{
			//either create identity matrix or zero matrix to filter out digital clock segments
			mat4 filterMatrix =	mat4((bitVec & vertBitVec) > 0 ? 1.f : 0.f);		//mat4(1.f) == identity matrix

			gl_Position = projection * view * model * filterMatrix * vertPos;
			uvs = vertUVs;
		}
	)";
	static const char*const  DigitalClockShader_instanced_vs = R"(
		#version 330 core
		layout (location = 0) in vec4 vertPos;				
		layout (location = 1) in vec2 vertUVs;				
		layout (location = 2) in int vertBitVec;

		//instanced propert				
		layout (location = 3) in int instancedBitVec;
				
		out vec2 uvs;

		uniform mat4 model = mat4(1.f);
		uniform mat4 view = mat4(1.f);
		uniform mat4 projection = mat4(1.f);

		void main()
		{
			//either create identity matrix or zero matrix to filter out digital clock segments
			mat4 filterMatrix =	mat4((instancedBitVec & vertBitVec) > 0 ? 1.f : 0.f);			//mat4(1.f) == identity matrix

			gl_Position = projection * view * model * filterMatrix * vertPos;
			uvs = vertUVs;
		}
	)";
	static const char*const DigitalClockShader_instanced_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		void main()
		{
			fragmentColor = vec4(1.f);
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

	void DigitalClockGlyph::render(struct GameUIRenderData& rd_ui, Shader& shader)
	{
		if (vao)
		{
			shader.use();

			ec(glBindVertexArray(vao));
			ec(glDrawArrays(GL_TRIANGLES, 0, vertex_positions.size()));
		}
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


		mat4 gsd_m = glm::scale(mat4(1.f), vec3(0.1f, 0.1f, 0.1f)); //global scale down
		//mat4 gsd_m = glm::scale(mat4(1.f), vec3(1.f)); //global scale down
		mat4 zero_m{ 0.f };

		//useful positions to composite together
		const float boxRadius = 8.f;
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
		mat4 horScale_m = scale({ boxLength,	1 });
		mat4 verScale_m = scale({ 1,			boxLength});

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
		translate_m = trans(bottomRowPos + leftColPos);
		transformAndAddVerts(gsd_m * translate_m, DCBars::LEFT_PERIOD);

		translate_m = trans(bottomRowPos);
		transformAndAddVerts(gsd_m * translate_m, DCBars::MIDDLE_PERIOD);

		translate_m = trans(bottomRowPos + rightColPos);
		transformAndAddVerts(gsd_m * translate_m, DCBars::RIGHT_PERIOD);

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
		}
	}


}

