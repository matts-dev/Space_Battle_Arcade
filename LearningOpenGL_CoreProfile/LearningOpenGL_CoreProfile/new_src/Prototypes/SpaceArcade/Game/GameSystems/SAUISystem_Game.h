#pragma once

#include <optional>
#include "../../GameFramework/SASystemBase.h"
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../../Tools/DataStructures/SATransform.h" //glm includes

struct GLFWwindow;

namespace SA
{
	struct GameUIRenderData;
	struct RenderData; //frame render data
	class CameraBase;
	class DigitalClockFont;

	////////////////////////////////////////////////////////
	// Game UI system
	////////////////////////////////////////////////////////
	class UISystem_Game : public SystemBase
	{
	public:
		void batchToDefaultText(DigitalClockFont& text, GameUIRenderData& ui_rd);
	private:
		friend class SpaceArcade;
		void runGameUIPass() const;
		virtual void initSystem() override;;
	public:
		mutable MultiDelegate<GameUIRenderData&> onUIGameRender;
	private:
		sp<DigitalClockFont> defaultTextBatcher = nullptr;
		mutable bool bRenderingGameUI = false;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shared render data to eliminate redundant calculations
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GameUIRenderData
	{
		template<typename T> using optional = std::optional<T>;
	public:
		float				dt_sec();
		glm::ivec2			framebuffer_Size();
		int					frameBuffer_MinDimension();
		glm::mat4			orthographicProjection_m();
		CameraBase*			camera();
		glm::vec3			camUp();
		glm::quat			camQuat();
		const RenderData*	renderData();
	private:
		void calculateFramebufferMetrics();
	private: //use accessors to lazy calculate these fields per invocation; allows sharing of data that has already been calculated
		optional<float>				_dt_sec;
		optional<glm::ivec2>		_framebuffer_Size;
		optional<int>				_frameBuffer_MinDimension;
		optional<glm::mat4>			_orthographicProjection_m;
		optional <sp<CameraBase> >	_camera;
		optional<glm::vec3>			_camUp;
		optional<glm::quat>			_camRot;
		optional<const RenderData*> _frameRenderData;
	};
}