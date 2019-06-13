#pragma once
#include <map>

#include "..\..\GameFramework\SALevel.h"

namespace SA
{
	class RenderModelEntity;
	class Shader;
	class PlayerBase;

	struct Projectile;
	

	class ProjectileEditor_Level : public Level
	{
	private:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void tick(float dt_sec) override;

		virtual void postConstruct() override;

		void handleUIFrameStarted();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int /*key*/, int /*state*/, int /*modifier_keys*/, int /*scancode*/);
		void handleButton(int /*button*/, int /*state*/, int /*modifier_keys*/);

		void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;

	private:
		bool bRenderModelTipOffset = false;
		bool bRenderCollisionBox = false;
		bool bRenderCollisionCenterOffset = false;
		bool bRenderCollisionBoxScaleup = false;
		/*scale the entire projectile, collision included*/
		float projectileScale = 1.0f;				
		float simulatedDeltaTime = 1 / 60.f;

		sp<RenderModelEntity> projectile;

		sp<Shader> forwardShaded_EmissiveModelShader;
		sp<Shader> debugLineShader;

	};
}
