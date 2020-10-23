#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include "../../Rendering/SAGPUResource.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "StarJumpData.h"
#include <string>
#include <glad/glad.h>

namespace SA
{
	class TexturedQuad;
	class Shader;
	class Texture_2D;
	struct RenderData;

	//represents an environmental object that looks like a nebula (ie space cloud)
	class Nebula : public GPUResource
	{
		using Parent = GPUResource;
	public:
		struct Data
		{
			std::string textureRelativePath;
			glm::vec3 tintColor{ 1.f };
		};
		Nebula(Data);
	protected:
		virtual void postConstruct() override;
	public:
		//static void primeShaderForRender(const RenderData& rd);
		void render(const RenderData& rd);
		void setOverrideData(Nebula::Data& overrideData);
		void setXform(Transform inXform, bool bRotateToOrigin=true);
		void setForceCentered(bool bInValue) { bForceCentered = bInValue; }
		const sp<Texture_2D>& getTexture() { return nebulaTexture; }
		void enableStarJump(bool bEnable, bool bSkipTransition = false){sj.enableStarJump(bEnable, bSkipTransition);}
	protected:
		virtual void onAcquireGPUResources() override;
		virtual void onReleaseGPUResources() override;
	private:
		void rotateRelativeToOrigin();
	private: //statics
		static sp<TexturedQuad> texturedQuad;
		static sp<Shader> circleFadeShader;
	private:
		sp<Texture_2D> nebulaTexture;
		bool bForceCentered = true;
		StarJumpData sj;
		Transform xform;
		GLuint nebulaTex = 0;
		Data data;
	};



}