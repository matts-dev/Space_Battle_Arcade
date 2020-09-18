#pragma once

#include "../SAGPUResource.h"
#include <glad/glad.h>
#include "../../Tools/DataStructures/SATransform.h"

namespace SA
{
	class Shader;
	class NdcQuad;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents a forward shaded HDR pipeline that is portable across different rendering systems.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ForwardRenderingStateMachine : public GPUResource 
	{
	public:
		using Parent = GPUResource;
	public:
		void begin_HdrStage(glm::vec3 clearColor);
		void begin_ToneMappingStage(glm::vec3 clearColor);
		bool IsUsingHDR() { return bEnableHDR; }
	protected:
		virtual void postConstruct() override;
		virtual void onReleaseGPUResources();
		virtual void onAcquireGPUResources();
	protected:
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleFramebufferResized(int width, int height);
	private:
		void framebuffer_allocate();
		void framebuffer_delete();
	private:
		int fb_width = 1;
		int fb_height = 1;
		bool bEnableHDR = true;
		bool bEnableBloom = true;
	private:
		GLuint fbo_hdr = 0;
		GLuint fbo_attachment_color_tex;
		GLuint fbo_attachment_depth_rbo;
		//bloom
		GLuint fbo_hdrThresholdExtraction;
		GLuint fbo_attachment_hdrExtractionColor;
		GLuint fbo_pingPong[2]; //perhaps should make a framebuffer object
		GLuint pingpongColorBuffers[2];

		sp<Shader> hdrColorExtractionShader = nullptr;
		sp<Shader> bloomShader = nullptr;
		sp<Shader> toneMappingShader = nullptr;
		sp<NdcQuad> ndcQuad = nullptr;
	};
}
