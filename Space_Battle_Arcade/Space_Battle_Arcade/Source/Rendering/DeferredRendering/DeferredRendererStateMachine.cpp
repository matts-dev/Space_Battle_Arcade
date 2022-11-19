#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SAPlayerBase.h"
#include "GameFramework/SAPlayerSystem.h"
#include "GameFramework/SARenderSystem.h"
#include "GameFramework/SAWindowSystem.h"
#include "Rendering/Camera/SACameraBase.h"
#include "Rendering/DeferredRendering/DeferredRendererStateMachine.h"
#include "Rendering/DeferredRendering/DeferredRenderingShaders.h"
#include "Rendering/Lights/PointLight_Deferred.h"
#include "Rendering/OpenGLHelpers.h"
#include "Rendering/RenderData.h"
#include "Rendering/SAShader.h"
#include "Tools/DataStructures/SATransform.h"
#include "Tools/Geometry/SimpleShapes.h"
#include "Tools/PlatformUtils.h"

//some good sources on implementing deferred rendering
// https://learnopengl.com/Advanced-Lighting/Deferred-Shading
// pt1 http://ogldev.atspace.co.uk/www/tutorial35/tutorial35.html
// pt2 http://ogldev.atspace.co.uk/www/tutorial36/tutorial36.html
// pt3 resources for setting up light volumes http://ogldev.atspace.co.uk/www/tutorial37/tutorial37.html

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// post processing
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* const postProcessShade_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 textureCoords;				
				out vec2 interpTextureCoords;				

				void main(){
					gl_Position = vec4(position, 1);
					interpTextureCoords = textureCoords;
				}
			)";
	const char* const postProcessShade_fs = R"(
				#version 330 core
				out vec4 fragmentColor;
				in vec2 interpTextureCoords;
				uniform sampler2D renderTexture;

				void main(){
					fragmentColor = texture(renderTexture, interpTextureCoords);
				}
			)";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Geometry Stage Shaders
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//moved to DeferredReneringShaders.h so they can be shared
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Lighting Stage Shader
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* const lbufferShader_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 texCoords;				

				uniform mat4 projection;
				uniform mat4 view;
				uniform mat4 model;
				
				out vec2 interpTexCoords;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					interpTexCoords = texCoords;
				}
			)";
	const char* const lbufferShader_PointLight_fs = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D albedo_specs;

				uniform vec3 camPos;
				uniform float width_pixels = 0;
				uniform float height_pixels = 0;
				
				struct PointLight
				{
					vec3 position;
					vec3 ambientIntensity;	vec3 diffuseIntensity;	vec3 specularIntensity;
					float constant;			float linear;			float quadratic;
				};	

				uniform PointLight pointLight; 
				
				void main(){
					//FIGURE OUT SCREEN COORDINATES
					//gl_FragCoord is window space, eg a 800x600 would be range [0, 800] and [0, 600]; 
					//so to convert to UV range (ie [0, 1]), we divide the frag_coord by the pixel values
					vec2 screenCoords = vec2(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels);

					//LOAD FROM GBUFFER (When rendering from sphere, we need to convert back to screen coords; alternatively this might could be done with perspective matrix but this way avoids matrix multiplication at expense of uniforms)
					vec3 fragPosition = texture(positions, screenCoords).rgb;
					vec3 fragNormal = normalize(texture(normals, screenCoords).rgb);
					vec3 color = texture(albedo_specs, screenCoords).rgb;
					float specularStrength = texture(albedo_specs, screenCoords).a;

					//STANDARD LIGHT EQUATIONS
					PointLight light = pointLight; 

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 toReflection = reflect(-toLight, fragNormal);
					vec3 toView = normalize(camPos - fragPosition);

					vec3 ambientLight = light.ambientIntensity * color;

					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * color;

					float specularAmount = pow(max(dot(toView, toReflection), 0), 32); //shinnyness will need to be embeded in a gbuffer
					vec3 specularLight = light.specularIntensity* specularAmount * specularStrength;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);

					fragmentColor = vec4(lightContribution, 0.0f);
		
					//debug getting appropriate screen coords
					//fragmentColor = vec4(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels, 0.0, 1.0f);
				}
			)";

	const char* const lbufferShader_DirectionalLight_fs = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D albedo_specs;

				uniform vec3 camPos;
				uniform float width_pixels = 0;
				uniform float height_pixels = 0;
				
				struct DirLight
				{
					vec3 dir_n;
					vec3 intensity;
				};	
				uniform DirLight dirLight; 
				
				void main(){
					//FIGURE OUT SCREEN COORDINATES
					//gl_FragCoord is window space, eg a 800x600 would be range [0, 800] and [0, 600]; 
					//so to convert to UV range (ie [0, 1]), we divide the frag_coord by the pixel values
					vec2 screenCoords = vec2(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels);
	
					//INIT
					vec3 toView = normalize(camPos - fragPosition);

					//LOAD FROM GBUFFER (When rendering from sphere, we need to convert back to screen coords; alternatively this might could be done with perspective matrix but this way avoids matrix multiplication at expense of uniforms)
					vec3 fragPosition = texture(positions, screenCoords).rgb;
					vec3 fragNormal = normalize(texture(normals, screenCoords).rgb);
					vec3 color = texture(albedo_specs, screenCoords).rgb;
					float specularStrength = texture(albedo_specs, screenCoords).a;

					//DIRECTIONAL LIGHT 
					vec3 dirDiffuse = vec3(0.f);
					vec3 dirSpecular = vec3(0.f);
					float n_dot_l = max(dot(fragNormal, -dirLight.dir_n), 0.f);
					dirDiffuse += color.rgb * n_dot_l * dirLight.intensity;

					//specular is a little ad-hoc since I do not currently define a specular component of the light for stars.
						//vec3 toSunLight_n = normalize(-directionalLightDir); //commenting htis out, should be using struct we passed.
					vec3 toSunLight_n = normalize(-dirLights[light].dir_n);
					float shininess = 32; //hard coded for now, needs to be embedded in gbuffer
					float specular = color.a; //spec packed into color within gbuffer
					float dirLightSpecAmount = pow(max(dot(toView, reflect(-toSunLight_n, fragNormal)), 0), shininess);
					dirSpecular +=  dirLights[light].intensity * dirLightSpecAmount* vec3(texture(material.texture_specular0, interpTextCoords));

					vec3 dirMapContrib = dirDiffuse + dirSpecular;

					vec3 lightContribution = dirMapContrib;

					fragmentColor = vec4(lightContribution, 0.0f);
		
					//debug getting appropriate screen coords
					//fragmentColor = vec4(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels, 0.0, 1.0f);
				}
			)";

	const char* const lbufferShader_AmbientLight_fs = R"(
				#version 330 core
				out vec4 fragmentColor;

				in vec2 interpTexCoords;

				uniform sampler2D positions;
				uniform sampler2D normals;
				uniform sampler2D albedo_specs;

				uniform vec3 camPos;
				uniform float width_pixels = 0;
				uniform float height_pixels = 0;
				uniform vec3 ambientIntensity = vec(0.1f);	
				
				uniform DirLight dirLight; 
				
				void main(){
					//FIGURE OUT SCREEN COORDINATES
					//gl_FragCoord is window space, eg a 800x600 would be range [0, 800] and [0, 600]; 
					//so to convert to UV range (ie [0, 1]), we divide the frag_coord by the pixel values
					vec2 screenCoords = vec2(gl_FragCoord.x / width_pixels, gl_FragCoord.y / height_pixels);
	
					//INIT
					vec3 toView = normalize(camPos - fragPosition);

					//LOAD FROM GBUFFER (When rendering from sphere, we need to convert back to screen coords; alternatively this might could be done with perspective matrix but this way avoids matrix multiplication at expense of uniforms)
					vec3 fragPosition = texture(positions, screenCoords).rgb;
					vec3 fragNormal = normalize(texture(normals, screenCoords).rgb);
					vec3 color = texture(albedo_specs, screenCoords).rgb;
					float specularStrength = texture(albedo_specs, screenCoords).a;

					//Ambient LIGHT 
					vec3 ambientLight = light.ambientIntensity * color;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);

					fragmentColor = vec4(lightContribution, 0.0f);
				}
			)";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Light Volume Stencil Marking Shader
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const char* const stencilWriter_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					//render to screen space so stencil buffer is filled in 
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* const stencilWriter_fs = R"(
				#version 330 core
				void main(){
					//do nothing, let stencil buffer be updated through glStencilFunc/glStencilOp
				}
			)";

	void DeferredRendererStateMachine::postConstruct()
	{
		Parent::postConstruct();

		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		windowSystem.onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &DeferredRendererStateMachine::handlePrimaryWindowChanging);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handlePrimaryWindowChanging(nullptr, primaryWindow);
		}

		sphereMesh = new_sp<SA::SphereMeshTextured>();

	}

	void DeferredRendererStateMachine::onAcquireGPUResources()
	{
		//Parent::onAcquireGPUResources();
		generateShaders();
		gbuffer_build();
		renderQuad_build();
	}

	void DeferredRendererStateMachine::generateShaders()
	{
		//regenerate shaders so that we know uniforms are set once we have opengl context information
			
		//a default shader to use if model does not provide one
		geometricStageShader = new_sp<Shader>(gbufferShader_vs, gbufferShader_fs, false);
		configureShaderForGBufferWrite(*geometricStageShader);

		//more efficient with complex scenes
		lightingStage_LightVolume_PointLight_Shader = new_sp<Shader>(lbufferShader_vs, lbufferShader_PointLight_fs, false);
		configureLightingShaderForGBufferRead(*lightingStage_LightVolume_PointLight_Shader);

		lightingStage_LightVolume_DirLight_Shader = new_sp<Shader>(lbufferShader_vs, lbufferShader_DirectionalLight_fs, false);
		configureLightingShaderForGBufferRead(*lightingStage_LightVolume_DirLight_Shader);

		lightingStage_LightVolume_AmbientLight_Shader = new_sp<Shader>(lbufferShader_vs, lbufferShader_AmbientLight_fs, false);
		configureLightingShaderForGBufferRead(*lightingStage_LightVolume_AmbientLight_Shader);

		stencilWriterShader = new_sp<Shader>(stencilWriter_vs, stencilWriter_fs, false);

		postProcessShader = new_sp<Shader>(postProcessShade_vs, postProcessShade_fs, false);
		postProcessShader->use();
		postProcessShader->setUniform1i("renderTexture", 0);
	}

	void DeferredRendererStateMachine::onReleaseGPUResources()
	{
		//Parent::onReleaseGPUResources();
		gbuffer_delete();
		renderQuad_delete();
	}

	void DeferredRendererStateMachine::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		//#todo #nextengine if we only ever have a single primary window... then why not put the framebuffer resizing events on the window system?
		if (old_window)
		{
			old_window->framebufferSizeChanged.removeWeak(sp_this(), &DeferredRendererStateMachine::handleFramebufferResized);
		}

		if (new_window)
		{
			new_window->framebufferSizeChanged.addWeakObj(sp_this(), &DeferredRendererStateMachine::handleFramebufferResized);
			std::pair<int, int> framebufferSize = new_window->getFramebufferSize();
			handleFramebufferResized(framebufferSize.first, framebufferSize.second); //this may not be safe if window isn't initialized, but probably should be
		}
	}

	void DeferredRendererStateMachine::handleFramebufferResized(int width, int height)
	{
		fbData.height = height;
		fbData.width = width;

		gbuffer_delete();
		gbuffer_build();
	}

	void DeferredRendererStateMachine::gbuffer_build()
	{
		if (hasAcquiredResources())
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// gbuffer
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (gbuffer)
			{
				STOP_DEBUGGER_HERE(); //what happened, system isn't expected to be building a gbuffer if one already exists?
				gbuffer_delete();
			}

			ec(glGenFramebuffers(1, &gbuffer));
			ec(glBindFramebuffer(GL_FRAMEBUFFER, gbuffer));

			//positions buffer
			ec(glGenTextures(1, &gAttachment_position));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_position));
			ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fbData.width, fbData.height, 0, GL_RGB, GL_FLOAT, nullptr));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			ec(glFramebufferTexture2D(GL_FRAMEBUFFER, /**/GL_COLOR_ATTACHMENT0/**/, GL_TEXTURE_2D, gAttachment_position, 0));

			//normals buffer
			ec(glGenTextures(1, &gAttachment_normals));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_normals));
			ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fbData.width, fbData.height, 0, GL_RGB, GL_FLOAT, nullptr));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			ec(glFramebufferTexture2D(GL_FRAMEBUFFER, /**/GL_COLOR_ATTACHMENT1/**/, GL_TEXTURE_2D, gAttachment_normals, 0));

			// ALBEDO (diffuse) + specular buffer
			ec(glGenTextures(1, &gAttachment_albedospec));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec));
			ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbData.width, fbData.height, 0, GL_RGBA, GL_FLOAT, nullptr));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			ec(glFramebufferTexture2D(GL_FRAMEBUFFER, /**/ GL_COLOR_ATTACHMENT2 /**/, GL_TEXTURE_2D, gAttachment_albedospec, 0));

			ec(glGenRenderbuffers(1, &gAttachment_depthStencil_RBO));
			ec(glBindRenderbuffer(GL_RENDERBUFFER, gAttachment_depthStencil_RBO));
			ec(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbData.width, fbData.height));
			ec(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gAttachment_depthStencil_RBO));

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating framebuffer for gbuffer: %x", error);
			}

			GLuint gbufferRenderTargets[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
			ec(glDrawBuffers(3, gbufferRenderTargets));

			ec(glBindRenderbuffer(GL_RENDERBUFFER, 0));
			ec(glBindFramebuffer(GL_FRAMEBUFFER, 0));

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// lbuffer
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//-------------------------------- LBUFFER ---------------------------------------------------
			ec(glGenFramebuffers(1, &lbuffer));
			ec(glBindFramebuffer(GL_FRAMEBUFFER, lbuffer));

			//positions buffer
			ec(glGenTextures(1, &lAttachment_lighting));
			ec(glBindTexture(GL_TEXTURE_2D, lAttachment_lighting));
			ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fbData.width, fbData.height, 0, GL_RGB, GL_FLOAT, nullptr));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			ec(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lAttachment_lighting, 0));

			ec(glGenRenderbuffers(1, &lAttachment_depthStencilRBO));
			ec(glBindRenderbuffer(GL_RENDERBUFFER, lAttachment_depthStencilRBO));
			ec(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbData.width, fbData.height));
			ec(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, lAttachment_depthStencilRBO));

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating lighting buffer: %x", error);
			}

		}

	}

	void DeferredRendererStateMachine::gbuffer_delete()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//note: deleting the textures also as to resize them to match new window dimensions
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (gbuffer)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// cleanup gbuffer
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			ec(glDeleteFramebuffers(1, &gbuffer));

			ec(glDeleteTextures(1, &gAttachment_position));
			ec(glDeleteTextures(1, &gAttachment_normals));
			ec(glDeleteTextures(1, &gAttachment_albedospec));
			ec(glDeleteRenderbuffers(1, &gAttachment_depthStencil_RBO));

			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// clean up lbuffer (lighting buffer) 
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			ec(glDeleteFramebuffers(1, &lbuffer));
			ec(glDeleteTextures(1, &lAttachment_lighting));
			ec(glDeleteRenderbuffers(1, &lAttachment_depthStencilRBO));
		}
	}

	void DeferredRendererStateMachine::renderQuad_build()
	{
		if (quadVAO)
		{
			STOP_DEBUGGER_HERE(); //why did we attempt to build a render quad if one already exists?!
			renderQuad_delete();
		}

		//create a render quad
		float quadVertices[] = {
			//x,y,z         s,t
			-1, -1, 0,      0, 0,
			1, -1, 0,       1, 0,
			1,  1, 0,       1, 1,

			-1, -1, 0,      0, 0,
			1,  1, 0,       1, 1,
			-1,  1, 0,      0, 1
		};
		numVertsInQuad = (sizeof(quadVertices) / sizeof(float)) / 5;

		ec(glGenVertexArrays(1, &quadVAO));
		ec(glBindVertexArray(quadVAO));

		ec(glGenBuffers(1, &quad_VBO));
		ec(glBindBuffer(GL_ARRAY_BUFFER, quad_VBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));

		ec(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))));
		ec(glEnableVertexAttribArray(1));

		ec(glBindVertexArray(0));
	}

	void DeferredRendererStateMachine::renderQuad_delete()
	{
		if (quadVAO)
		{
			//hmm, may should check that we don't have the VAO bound
			ec(glDeleteVertexArrays(1, &quadVAO));
			ec(glDeleteBuffers(1, &quad_VBO));
		}

	}

	void DeferredRendererStateMachine::beginGeometryPass(glm::vec3 renderClearColor)
	{
		//restore defaults of culling
		ec(glEnable(GL_CULL_FACE));
		ec(glCullFace(GL_BACK));

		//enable stencil so we can clear any lingering writes
		ec(glEnable(GL_STENCIL_TEST)); //enabling to ensure that we clear stencil every frame (may not be necessary)
		ec(glStencilMask(0xff)); //enable complete stencil writing so that clear will clear stencil buffer (also, not tested if necessary)

		//clear the gbuffer for rendering
		ec(glBindFramebuffer(GL_FRAMEBUFFER, gbuffer));
		ec(glClearColor(renderClearColor.r, renderClearColor.g, renderClearColor.b, 1));
		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

	}

	void DeferredRendererStateMachine::beginLightPass()
	{
		RenderSystem& renderSystem = GameBase::get().getRenderSystem();
		const std::vector<sp<PointLight_Deferred>>& framePointLights = renderSystem.getFramePointLights();
		const RenderData* frd = renderSystem.getFrameRenderData_Read(GameBase::get().getFrameNumber());

		if (frd 
			&& lightingStage_LightVolume_PointLight_Shader 
			&& lightingStage_LightVolume_DirLight_Shader
			&& stencilWriterShader 
			&& sphereMesh 
			&& frd->playerCamerasPositions.size() > 0 )
		{
			glm::vec3 camPos = frd->playerCamerasPositions[0];//#TODO #splitscreen needs to handle multiple renders from different camera locations, perhaps it should be 
			int width = fbData.width;
			int height = fbData.height;
			const glm::mat4& view = frd->view;
			const glm::mat4& projection = frd->projection;

			ec(glBindFramebuffer(GL_FRAMEBUFFER, lbuffer));

			ec(glActiveTexture(GL_TEXTURE0));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_position));

			ec(glActiveTexture(GL_TEXTURE1));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_normals));

			ec(glActiveTexture(GL_TEXTURE2));
			ec(glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec));

			//clear the lighting frame buffer
			ec(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
			ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			//---- COPY DEPTH ---- copy over depth information so lights do not render on top of deferred shading objects
			//this is needed for forward shading component and for stencil testing light volumes
			//		FYI tutorial warns about this type of copying might not be portable when copying to/from default framebuffer (ie 0) because the depth buffer formats must match
			//		(this si fine because we're copying between two FBs that we know match in terms of depth buffer configuration)
			ec(glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer));
			ec(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer));
			ec(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST));
			ec(glBindFramebuffer(GL_FRAMEBUFFER, lbuffer));
			//----------------

			ec(glDepthMask(GL_FALSE));
			ec(glEnable(GL_STENCIL_TEST));

			//set up blending so that we can accumulate light; each pass is a single light. so we want to add the new light on top of the previous light!
			ec(glEnable(GL_BLEND));
			ec(glBlendFunc(GL_ONE, GL_ONE)); //set source and destination factors, recall blending is color = source*sourceFactor + destination*destinationFactor;

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Point Lights
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			auto configureLightShader = [&](Shader& lightShader) {
				lightShader.use();
				lightShader.setUniform3f("camPos", camPos);
				lightShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
				lightShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				lightShader.setUniform1f("width_pixels", (float)width);
				lightShader.setUniform1f("height_pixels", (float)height);
			};
			configureLightShader(*lightingStage_LightVolume_PointLight_Shader);

			stencilWriterShader->use();
			stencilWriterShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			stencilWriterShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));

			//perhaps we should do a processing step that gets all point light data ready, and sorted, so we can only render a set number
			for (size_t idx = 0; idx < framePointLights.size(); ++idx)
			{
				if (const sp<PointLight_Deferred>& light = framePointLights[idx])
				{
					if (light->getSystemData().bUserDataDirty) { light->clean(); }

					//select between debug radius and real radius like a ternary
					float lightRadius = light->getSystemData().maxRadius*float(!bDebugLightVolumes) + (1.f* float(bDebugLightVolumes));

					//note: lights have been disabled as arrays for my light volumes; so they must be updated one at a time.
					glm::mat4 sphereModelMatrix;
					sphereModelMatrix = glm::translate(sphereModelMatrix, light->getUserData().position);
					sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(lightRadius));

					stencilWriterShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));

					//------STENCIL PASS---------
					ec(glDisable(GL_CULL_FACE));			//make sure we process back faces, we need them to increment stencil (like below image)
					ec(glClearStencil(0));					//sets value for clearing stencil, for debugging you can change this value 
					ec(glStencilMask(0xFF));				//enable writing to stencil buffer
					ec(glClear(GL_STENCIL_BUFFER_BIT));		//clear stencil before we mark volume
					ec(glStencilFunc(GL_ALWAYS, 0, 0xFF)); //always write stencil results

					// set it up so stencil is only written on depth failures, consider below to understand the set up.
					//  light volume sphere                                   X
					//      _____			        _____	                 _____			            _____
					//    +  +1  +			      +  +1   +	               +   +0  +	              +   +1  +
					//  +          +		    +          +             +          +		        +     X    +
					// |            |		   |     X      |           |            |		       |            |
					//  +          +		    +          +             +          +		    --- +     vp   +  -----
					//    +  -1   +			      +       +	               +   +0  +		 	|     +      + not    |
					//      -----			        -----	                 -----			    |behind ----- rendered|
					//        X                                                                 |_____________________|
					//      vp                       vp                       vp
					//
					// X is object; vp is viewer's position. When stencil is 0, no lighting is applied
					//
					// stencil = 0;                stencil = +1              stencil = 0               stencil = +1
					ec(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP)); //note this is "DEPTH FAILS", less intuitive than depth pass but works better
					ec(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP)); //note: stencil does not have negative numbers according to spec

					stencilWriterShader->use();
					sphereMesh->render();

					//------LIGHTING PASS--------
					ec(glDisable(GL_DEPTH_TEST)); //don't allow depth to stop rendering a sphere
					ec(glEnable(GL_CULL_FACE));
					ec(glCullFace(GL_FRONT)); //use back of sphere for lighting so that if camera is inside sphere lighting is still rendered
					lightingStage_LightVolume_PointLight_Shader->use();
					lightingStage_LightVolume_PointLight_Shader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
					light->applyUniforms(*lightingStage_LightVolume_PointLight_Shader); //make this not a function of light? assumes a lot about uniform names

					ec(glStencilMask(0)); //disable writing
					ec(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
					ec(glStencilFunc(GL_NOTEQUAL, 0, 0xFF)); //only write if passed behind front_face (-1) and infront of  back_face(+1); (-1 + 1 = 0);
					sphereMesh->render(); 
					ec(glEnable(GL_DEPTH_TEST)); //reenable
				}
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// clean up point light set up 
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			ec(glDisable(GL_STENCIL_TEST));
			ec(glDepthMask(GL_TRUE));

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Direction Lighting
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//note: this stencil clear may need reconfiguring
			ec(glClearStencil(0));					//sets value for clearing stencil, for debugging you can change this value 
			ec(glStencilMask(0xFF));				//enable writing to stencil buffer
			ec(glClear(GL_STENCIL_BUFFER_BIT));		//clear stencil before we mark volume
			configureLightShader(*lightingStage_LightVolume_DirLight_Shader);
			lightingStage_LightVolume_DirLight_Shader->use();

			for (const DirectionLight& dirLight : frd->dirLights)
			{
				lightingStage_LightVolume_DirLight_Shader->setUniform3f("dirLight.dir_n", dirLight.direction_n);
				lightingStage_LightVolume_DirLight_Shader->setUniform3f("dirLight.intensity", dirLight.lightIntensity);

				//render full screen quad so that directional light is applied to entire screen
				ec(glBindVertexArray(quadVAO));
				ec(glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad));
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// ambient light
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			configureLightShader(*lightingStage_LightVolume_AmbientLight_Shader);
			lightingStage_LightVolume_AmbientLight_Shader->use();
			lightingStage_LightVolume_AmbientLight_Shader->setUniform3f("ambientIntensity", frd->ambientLightIntensity);
			ec(glBindVertexArray(quadVAO));
			ec(glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad));

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// clean up
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			ec(glDisable(GL_BLEND));
			ec(glCullFace(GL_BACK));
		}
	}

	void DeferredRendererStateMachine::beginPostProcessing()
	{
		ec(glBindFramebuffer(GL_FRAMEBUFFER, 0)); //make primary frame buffer ac tive

		ec(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		ec(glActiveTexture(GL_TEXTURE0));

		switch (displayBuffer)
		{
		case BufferType::NORMAL:
			glBindTexture(GL_TEXTURE_2D, gAttachment_normals);
			break;
		case BufferType::POSITION:
			glBindTexture(GL_TEXTURE_2D, gAttachment_position);
			break;
		case BufferType::ALBEDO_SPEC:
			glBindTexture(GL_TEXTURE_2D, gAttachment_albedospec);
			break;
		case BufferType::LIGHTING:
			glBindTexture(GL_TEXTURE_2D, lAttachment_lighting);
			break;
		}

		postProcessShader->use();

		ec(glBindVertexArray(quadVAO));
		ec(glDrawArrays(GL_TRIANGLES, 0, numVertsInQuad));

	}

	void DeferredRendererStateMachine::configureShaderForGBufferWrite(Shader& geometricStageShader)
	{
#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
		TODO_this_function_is_a_lie_qark____this_is_just_configuring_texture_locations_in_a_material____it_is_not_configuring_framebuffer_locations;
		todo_the_frame_buffer_setup_is_done_in_fs_with_layout_location_0_out_vec3_position; this_can_be_removed_qmark;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

		geometricStageShader.use();
		geometricStageShader.setUniform1i("material.texture_diffuse0", 0);
		geometricStageShader.setUniform1i("material.texture_specular0", 1);
	}

	void DeferredRendererStateMachine::configureLightingShaderForGBufferRead(Shader& lightingStage_LightVolume_Shader)
	{
		lightingStage_LightVolume_Shader.use();
		lightingStage_LightVolume_Shader.setUniform1i("positions", 0);
		lightingStage_LightVolume_Shader.setUniform1i("normals", 1);
		lightingStage_LightVolume_Shader.setUniform1i("albedo_specs", 2);
	}

}

