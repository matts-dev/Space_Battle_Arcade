#pragma once

#include "Rendering/SAGPUResource.h"
#include <glad/glad.h>
#include "Tools/DataStructures/SATransform.h"

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
		void stage_HDR(glm::vec3 clearColor);
		void stage_ToneMapping(glm::vec3 clearColor);
		void stage_MSAA();
	public:
		bool isUsingHDR() { return bEnableHDR; }
		bool isUsingMultiSample() { return bMultisampleEnabled; }
		void setUseMultiSample(bool bEnableMultiSample);
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
		bool bMultisampleEnabled = false; //currently only works for UI and laser lines do not look as good IMO, disable by default.
	private:
		GLuint fbo_hdr = 0;
		GLuint fbo_attachment_color_tex;
		GLuint fbo_attachment_depth_rbo;
		//bloom
		GLuint fbo_hdrThresholdExtraction;
		GLuint fbo_attachment_hdrExtractionColor;
		GLuint fbo_pingPong[2]; //perhaps should make a framebuffer object
		GLuint pingpongColorBuffers[2];
		//msaa
		GLuint fbo_multisample = 0;
		GLuint fbo_multisample_color_attachment;
		GLuint fbo_multisample_depthstencil_rbo;
		const GLuint samples = 4; //note, this is hardcoded in offscreen msaa shaders, needs to be adjusted there too, one can not simply adjust this number

		sp<Shader> hdrColorExtractionShader = nullptr;
		sp<Shader> bloomShader = nullptr;
		sp<Shader> toneMappingShader = nullptr;
		sp<Shader> MSAA_Shader = nullptr;
		sp<NdcQuad> ndcQuad = nullptr;
	};
}
