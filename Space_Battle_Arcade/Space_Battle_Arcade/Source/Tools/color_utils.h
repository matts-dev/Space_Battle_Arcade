#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SA
{
	using ColorType = glm::vec3; //attempt include this since colors below have issue finding include path for VAX smart-include

	namespace color
	{
		glm::vec3 rgb(uint32_t hex);
		glm::vec4 rgba(uint32_t hex);

		inline glm::vec3 metalicgold() { return rgb(0xD4AF37); }
		inline glm::vec3 emerald() { return rgb(0x50c878); }
		inline glm::vec3 emeraldDark() { return rgb(0x27592D); }
		inline glm::vec3 brightGreen() { return rgb(0x80FF00); }
		inline glm::vec3 brightPink() { return rgb(0xFF9999); }
		inline glm::vec3 purple() { return rgb(0x7F00FF); }
		inline glm::vec3 red() { return rgb(0xFF0000); }
		inline glm::vec3 green() { return rgb(0x00FF00); }
		inline glm::vec3 blue() { return rgb(0x0000FF); }
		inline glm::vec3 lightYellow() { return rgb(0xFFFF66); }
		inline glm::vec3 lightOrange() { return rgb(0xFF9933); }
		inline glm::vec3 lightPurple() { return rgb(0xFF33FF); }

		
	}
}
