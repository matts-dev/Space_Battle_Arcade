#include "color_utils.h"

namespace SA
{
	namespace color
	{
		glm::vec3 rgb(uint32_t hex)
		{
			uint32_t r = (hex & 0xFF0000) >> 16;
			uint32_t g = (hex & 0xFF00) >> 8;
			uint32_t b = (hex & 0xFF);

			return glm::vec3(r / 255.f, g / 255.f, b / 255.f);
		}

		glm::vec4 rgba(uint32_t hex)
		{
			uint32_t r = (hex & 0xFF000000) >> 24;
			uint32_t g = (hex & 0xFF0000) >> 16;
			uint32_t b = (hex & 0xFF00) >> 8;
			uint32_t a = (hex & 0xFF);

			return glm::vec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
		}

	}
}
