#pragma once
#include "SAGPUResource.h"
#include <glad/glad.h>

namespace SA
{

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//A quad that has the coordinates of NDC, which are [-1,1] rather than [-0.5f, 0.5f]
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class NdcQuad : public GPUResource
	{
	public:
		void render();
	private:
		virtual void onAcquireGPUResources();
		virtual void onReleaseGPUResources();
	private:
		size_t numVerts = 6;
	private:
		GLuint quad_VAO;
		GLuint quad_VBO;
	};

















	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Default vertex shaders for post processing
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* const postProcessForwardShader_default_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec2 textureCoords;				

		out vec2 interpTextureCoords;				

		void main(){
			gl_Position = vec4(position, 1);
			interpTextureCoords = textureCoords;
		}
	)";



	const char* const toneMappingForwardShader_default_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		in vec2 interpTextureCoords;
		uniform sampler2D renderTexture;
		uniform sampler2D gaussianBlur;

		uniform bool bEnableHDR = true;		
		uniform bool bEnableBloom = true;
				
		void main(){
			if(bEnableHDR){
				vec3 hdrColor = texture(renderTexture, interpTextureCoords).rgb;

				if(bEnableBloom)
				{
					vec3 blurColor = texture(gaussianBlur, interpTextureCoords).rgb;
					hdrColor += blurColor;
				}		

				//reinhard tone mapping
				//vec3 toneMapped = (hdrColor) / (1 + hdrColor);

				// exposure tone mapping
				float exposure = 1.0f;
				vec3 toneMapped = vec3(1.0) - exp(-hdrColor * exposure);

				//correct for gamma (***textures may need to be loaded with sRGB for accurate visual results; depends on how they were constructed***)
				float gamma = 2.2f;
				//toneMapped  = pow(toneMapped, vec3(1.0f / gamma));

				fragmentColor = vec4(toneMapped, 1.0f);

			} else {
				fragmentColor = texture(renderTexture, interpTextureCoords);
			}
		}
	)";


	
	const char* const hdrExtraction_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		in vec2 interpTextureCoords;
		uniform sampler2D renderTexture;

		void main()
		{
			vec3 hdrColor = texture(renderTexture, interpTextureCoords).rgb;
				
			vec3 grayScaleThreshold = vec3(0.2126, 0.7152, 0.0722);
			float thresholdCheck = dot(grayScaleThreshold, vec3(hdrColor));
						
			float brightnessThreshold = 1.0f;
			if(thresholdCheck > brightnessThreshold){
				fragmentColor = vec4(hdrColor.rgb, 1);
			} else {
				fragmentColor = vec4(0,0,0, 1);
			}
		}
	)";

	const char* const bloomShader_gausblur_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		in vec2 interpTextureCoords;

		uniform sampler2D image;
		uniform bool horizontalBlur = false;
		uniform float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
				
		void main(){
			vec2 texelSize = 1.0f / textureSize(image, 0);
			vec3 color = texture(image, interpTextureCoords).rgb;
			color *= weights[0];

			if(horizontalBlur)
			{
				//**** note these loops start 1 -- because we've already used weight 0 for center ****`
				for(int texel_idx = 1; texel_idx < 5; ++texel_idx)
				{
					//calculate mirrored texels at same time
					color += texture(image, interpTextureCoords + vec2(texelSize.x * texel_idx, 0.0f)).rgb * weights[texel_idx];	
					color += texture(image, interpTextureCoords + vec2(-texelSize.x * texel_idx, 0.0f)).rgb * weights[texel_idx];	
				}
			} 
			else 
			{
				for(int texel_idx = 1; texel_idx < 5; ++texel_idx)
				{
					color += texture(image, interpTextureCoords + vec2(0.0f, texelSize.y * texel_idx)).rgb * weights[texel_idx];	
					color += texture(image, interpTextureCoords + vec2(0.0f, -texelSize.y * texel_idx)).rgb * weights[texel_idx];								
				}					
			}

			fragmentColor = vec4(color, 1.0f);
			//fragmentColor = vec4(texture(image, interpTextureCoords).rgb, 1.f);
		}
	)";

}