#pragma once
#include "..\..\GameFramework\SALevel.h"


namespace SA
{
	class BaseLevel : public Level
	{

	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;

	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

	protected:
		sp<SA::Shader> forwardShadedModelShader;
	};
}