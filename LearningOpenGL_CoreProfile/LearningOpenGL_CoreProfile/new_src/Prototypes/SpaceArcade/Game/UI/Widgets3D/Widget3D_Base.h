#pragma once
#include "../../../GameFramework/Components/SAComponentEntity.h"
#include "../../../Tools/DataStructures/SATransform.h"
#include <optional>

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shared render data to elminate redundent calculations
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GameUIRenderData
	{
		template<typename T> using optional = std::optional<T>;
	public:
		float		dt_sec();
		glm::ivec2	framebuffer_Size();
		int			frameBuffer_MinDimension();
		glm::mat4	orthographicProjection_m();
	private:
		void calculateFramebufferMetrics();
	private: //use accessors to lazy calculate these fields per invocation; allows sharing of data that has already been calculated
		optional<float>			_dt_sec;
		optional<glm::ivec2>	_framebuffer_Size;
		optional<int>			_frameBuffer_MinDimension;
		optional<glm::mat4>		_orthographicProjection_m;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Widget 3D base class
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_Base : public SystemComponentEntity
	{
	public:
		/** Mutable parameter is to allow lazy calculation*/
		virtual void render(GameUIRenderData& renderData) = 0;
	};

}