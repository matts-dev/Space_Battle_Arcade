#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include<cstdint>
#include <glad/glad.h>
#include <array>
#include <GLFW/glfw3.h>

namespace Utility
{
	static const float clockRectComponent[] = 
	{
		//x     y		      u		v
		-1.0f, -1.0f,       0.0f, 0.0f,
		1.0f, -1.0f,		1.0f, 0.0f,
		1.0f, 1.0f,			1.0f, 1.0f,

		-1.0f, -1.0f,       0.0f, 0.0f,
		1.0f, 1.0f,         1.0f, 1.0f,
		-1.0f, 1.0f,        1.0f, 0.0f
	};

	char const * const DigitalClockShader_Vertex = R"(
		#version 330 core
		layout (location = 0) in vec2 position;
		layout (location = 1) in vec2 uv;
		out vec2 fragUV;
		uniform mat4 transform;

		void main()
		{
			//uvs provided in for custom frag shaders that might use textures
			fragUV = uv;
			gl_Position = transform * vec4(position, 0.0f, 1.0f);
		}
	)";

	char const * const DigitalClockShader_Fragment = R"(
		#version 330 core
		in vec2 fragUV;		
		out vec4 fragColor;
		uniform vec3 color = vec3(1.0f, 1.0f, 1.0f);
		void main()
		{
			fragColor = vec4(color, 1.0f);
		}
	)";

	enum class DCBars : uint8_t
	{
		TOP = 0,
		MIDDLE,
		BOTTOM,
		LEFT_TOP,
		LEFT_BOTTOM,
		RIGHT_TOP,
		RIGHT_BOTTOM,
	};



	/** Invariant: Opengl has been set up before construction of this class! */
	struct DigitalClockNumber
	{
		std::array<bool[7], 10> numberMapping = 
		{
			/*          top	  middle  bottom left_top left_bottom right_top right_bottom */
			/* 0 */		true,  false, true,		true,	true,		true,	true,
			/* 1 */		false,	false,	false,	false,	false,		true,	true,
			/* 2 */		true,	true,	true,	false,	true,		true,	false,
			/* 3 */		true,	true,	true,	false,	false,		true,	true,
			/* 4 */		false,	true,	false,	true,	false,		true,	true,
			/* 5 */		true,	true,	true,	true,	false,		false,	true,
			/* 6 */		true,	true,	true,	true,	true,		false,	true,
			/* 7 */		true,	false,	false,	false,	false,		true,	true,
			/* 8 */		true,	true,	true,	true,	true,		true,	true,	
			/* 9 */		true,	true,	false,	true,	false,		true,	true
		};

		uint32_t number;

		DigitalClockNumber(uint32_t inNumber = 0) : 
			number(inNumber)
		{
			using glm::mat4; using glm::vec4; using glm::vec3;

			glGenVertexArrays(1, &VAO);
			glBindVertexArray(VAO);

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(clockRectComponent), clockRectComponent, GL_STATIC_DRAW);

			//Positions
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
			glEnableVertexAttribArray(0);

			//UVs
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
			glEnableVertexAttribArray(1);


			//set up transforms
			mat4 makeSquareUnitScale = glm::scale(mat4(1.0f), vec3(0.1f, 0.1f, 0.1f));
			mat4 horizontalScale = glm::scale(makeSquareUnitScale, vec3(3, 1, 1));
			mat4 verticalScale = glm::scale(makeSquareUnitScale, vec3(1, 3, 1));

			//horizontal bars
			localBarTransforms[static_cast<uint8_t>(DCBars::TOP)] = glm::translate(mat4(1.0f), vec3(0.0f, 1.0f - 0.125f, 0.0f)) * horizontalScale; //top 8th
			localBarTransforms[static_cast<uint8_t>(DCBars::MIDDLE)] = horizontalScale; //middle
			localBarTransforms[static_cast<uint8_t>(DCBars::BOTTOM)] = glm::translate(mat4(1.0f), vec3(0.0f, -1.0f + 0.125f, 0.0f)) * horizontalScale; //bottom 8th

			//vertical bars
			localBarTransforms[static_cast<uint8_t>(DCBars::LEFT_TOP)] =		glm::translate(mat4(1.0f), vec3(-0.45f, 0.45f, 0.0f)) * verticalScale;
			localBarTransforms[static_cast<uint8_t>(DCBars::LEFT_BOTTOM)] =		glm::translate(mat4(1.0f), vec3(-0.45f, -0.45f, 0.0f)) * verticalScale;
			localBarTransforms[static_cast<uint8_t>(DCBars::RIGHT_TOP)] =		glm::translate(mat4(1.0f), vec3(0.45f, 0.45f, 0.0f)) * verticalScale;
			localBarTransforms[static_cast<uint8_t>(DCBars::RIGHT_BOTTOM)] =	glm::translate(mat4(1.0f), vec3(0.45f, -0.45f, 0.0f)) * verticalScale;
		}

		~DigitalClockNumber()
		{
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		}

		void render(const glm::mat4& screenSpaceXform, GLuint ShaderId)
		{
			glUseProgram(ShaderId);
			int uniformLocation = glGetUniformLocation(ShaderId, "transform");
			
			
			glBindVertexArray(VAO);

			//draw top
			if (numberMapping[number][static_cast<uint32_t>(DCBars::TOP)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::TOP)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::MIDDLE)])
			{
			glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::MIDDLE)]));
			glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::BOTTOM)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::BOTTOM)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::RIGHT_TOP)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::RIGHT_TOP)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::RIGHT_BOTTOM)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::RIGHT_BOTTOM)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::LEFT_TOP)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::LEFT_TOP)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			if (numberMapping[number][static_cast<uint32_t>(DCBars::LEFT_BOTTOM)])
			{
				glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(screenSpaceXform * localBarTransforms[static_cast<uint8_t>(DCBars::LEFT_BOTTOM)]));
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			glBindVertexArray(0);
		}

	private:
		void DrawCube()
		{

		}

	private:
		GLuint VAO, VBO;
		std::array<glm::mat4, 7> localBarTransforms;

	};


	class FrameRateDisplay
	{
	public:
		FrameRateDisplay(const glm::vec2& screenspaceLoc = glm::vec2(-0.9f, 0.9f))
		{
			xformNDC = glm::translate(glm::mat4(1.0f), glm::vec3(screenspaceLoc, 0.0f));
		}
		~FrameRateDisplay(){}

		void tick()
		{
			static float lastFrameTime = static_cast<float>(glfwGetTime());

			float currentTime = static_cast<float>(glfwGetTime());
			float deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			uint32_t fps = static_cast<uint32_t>(1.0f / deltaTime);
			fps = fps > 999 ? 999 : fps;

			ones = fps % 10;
			fps -= ones;

			tens = fps % 100;
			fps -= tens;

			hundreds = fps % 1000;
			fps -= hundreds;

			hundreds /= 100;
			tens /= 10;

		}

		void render(const glm::ivec2& windowSize, GLuint shaderId)
		{
			using glm::mat4; using glm::vec3;

			const float downScaleFactor = 0.025f;
			mat4 downScale = glm::scale(mat4(1.0f), vec3(downScaleFactor));

			mat4 hundredsXform = xformNDC * glm::translate(mat4(1.0f), vec3(-(downScaleFactor + 0.5f*downScaleFactor), 0.0f, 0.0f)) *downScale;
			mat4 tensXform = xformNDC * mat4(1.0f) * downScale;
			mat4 onesXform = xformNDC * glm::translate(mat4(1.0f), vec3((downScaleFactor + 0.5f*downScaleFactor), 0.0f, 0.0f)) * downScale;

			numberRenderer.number = hundreds;
			numberRenderer.render(hundredsXform, shaderId);

			numberRenderer.number = tens;
			numberRenderer.render(tensXform, shaderId);

			numberRenderer.number = ones;
			numberRenderer.render(onesXform, shaderId);
		}
	private:
		glm::mat4 xformNDC;
		uint32_t ones;
		uint32_t tens;
		uint32_t hundreds;
		DigitalClockNumber numberRenderer;
	};

	
}
