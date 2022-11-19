#pragma once
#include "Rendering/SAGPUResource.h"
#include <glad/glad.h>
#include <fwd.hpp>

namespace SA
{
	class SphereMeshTextured;
	class Shader;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Embodies the state and transitions of a deferred renderer.
	//
	// Deferred rendering renders all geometry data to different textures within a gbuffer (framebuffer); this is the geometry pass
	// After the geometry pass, a lighting pass is done where many lights are rendered using the data available in the gbuffer.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class DeferredRendererStateMachine : public GPUResource
	{
	public:
		using Parent = GPUResource;
		enum class BufferType : uint8_t{ NORMAL, POSITION, ALBEDO_SPEC, LIGHTING};
	public:
		void beginGeometryPass(glm::vec3 gbufferClearColor);
		void beginLightPass();
		void beginPostProcessing();
	public:
		void setDisplayBuffer(BufferType buffer) { displayBuffer = buffer; };
		static void configureShaderForGBufferWrite(Shader& geometricStageShader);
		static void configureLightingShaderForGBufferRead(Shader& lightingStageShader);
	protected:
		virtual void postConstruct() override;
		virtual void onReleaseGPUResources() override;
		virtual void onAcquireGPUResources() override;
	protected:
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleFramebufferResized(int width, int height);
	private:
		void generateShaders();
		void gbuffer_build();
		void gbuffer_delete();
		void renderQuad_build();
		void renderQuad_delete();
	private:
		//geometry buffer and attachments
		GLuint gbuffer = 0;
		GLuint gAttachment_position = 0;
		GLuint gAttachment_normals = 0;
		GLuint gAttachment_albedospec = 0;
		GLuint gAttachment_depthStencil_RBO = 0;

		//lighting buffer and attachments
		GLuint lbuffer = 0;
		GLuint lAttachment_lighting = 0;
		GLuint lAttachment_depthStencilRBO = 0;

		//render quad (sized to match NDC)
		GLuint quadVAO = 0;
		GLuint quad_VBO = 0;
		size_t numVertsInQuad = 0;

		sp<Shader> geometricStageShader = nullptr;
		sp<Shader> lightingStage_LightVolume_PointLight_Shader = nullptr;
		sp<Shader> lightingStage_LightVolume_DirLight_Shader = nullptr;
		sp<Shader> lightingStage_LightVolume_AmbientLight_Shader = nullptr;
		sp<Shader> stencilWriterShader = nullptr; //writes light volume locations to stencil buffer 
		sp<Shader> postProcessShader = nullptr;
		sp<SphereMeshTextured> sphereMesh = nullptr;

		bool bDebugLightVolumes = false;
	private:
		struct WindowFrameBufferData
		{
			int width = 1;
			int height = 1;
		} fbData = {};

		BufferType displayBuffer = BufferType::LIGHTING;
	};
}