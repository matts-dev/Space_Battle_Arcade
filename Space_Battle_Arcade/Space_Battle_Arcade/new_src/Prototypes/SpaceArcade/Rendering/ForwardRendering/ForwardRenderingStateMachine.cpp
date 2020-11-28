 #include "ForwardRenderingStateMachine.h"
 #include "../../GameFramework/SALog.h"
 #include "../OpenGLHelpers.h"
 #include "../../GameFramework/SAWindowSystem.h"
 #include "../../Tools/PlatformUtils.h"
 #include "../NdcQuad.h"
 #include "../SAShader.h"
 #include "../../GameFramework/SAGameBase.h"
 
 namespace SA
 {
 	void ForwardRenderingStateMachine::stage_HDR(glm::vec3 clearColor)
 	{
 		ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_hdr));
 
 		//clear the HDR frame buffer
 		ec(glEnable(GL_DEPTH_TEST));
 		ec(glEnable(GL_STENCIL_TEST)); //enabling to ensure that we clear stencil every frame (may not be necessary) //#TODO test if necessary for clear
 		ec(glStencilMask(0xff)); //enable complete stencil writing so that clear will clear stencil buffer (also, not tested if necessary) 
 		ec(glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f));
 		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
 
 		ec(glStencilMask(0x0)); //we cleared stencil buffer, stop writing to stencil buffer.
 		ec(glDisable(GL_STENCIL_TEST)); //only enable stencil test on demand
 	}
 
 	void ForwardRenderingStateMachine::stage_ToneMapping(glm::vec3 clearColor)
 	{
		if (fb_height == 0 || fb_width == 0)
		{
			return; //user has minimized.
		}

 		//draw render quad (this is where HDR calculations are done)
 
 		//enable screen's frame buffer
 		ec(glBindFramebuffer(GL_FRAMEBUFFER, bMultisampleEnabled ? fbo_multisample : 0)); //select between default buffer and multisample buffer
 
 		//clear the frame buffer
 		ec(glEnable(GL_DEPTH_TEST));
 		ec(glEnable(GL_STENCIL_TEST)); //enabling to ensure that we clear stencil every frame (may not be necessary)
 		ec(glStencilMask(0xff)); //enable complete stencil writing so that clear will clear stencil buffer (also, not tested if necessary)
 		ec(glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f)); 
 
 		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
 
 		ec(glActiveTexture(GL_TEXTURE0));
 		ec(glBindTexture(GL_TEXTURE_2D, fbo_attachment_color_tex));
 
 		ec(glDisable(GL_STENCIL_TEST)); //disable until needed
 		
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		// begin post processing
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		ec(glDisable(GL_DEPTH_TEST));  //don't care about depth for post process, also don't write quad depth
 
 		////////////////////////////////////////////////////////
 		// bloom
 		////////////////////////////////////////////////////////
 		if (bEnableHDR && bEnableBloom)
 		{
 			////////////////////////////////////////////////////////
 			// find bright colors pass 
 			// (because we didn't do this in all model shaders)
 			////////////////////////////////////////////////////////
 			ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_hdrThresholdExtraction));
 			ec(glActiveTexture(GL_TEXTURE0));
 			ec(glBindTexture(GL_TEXTURE_2D, fbo_attachment_color_tex)); //read from the base fbo color attachment to cut out HDR lighting that should be blurred
 			ec(glClearColor(0.0f, 0.0f, 0.0f, 1.0f)); //we should clear black as we're going to be adding these colors together, we don't want to double add render clear color
 			ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
 			hdrColorExtractionShader->use();
 			hdrColorExtractionShader->setUniform1i("renderTexture", 0);
 			ndcQuad->render(); //this renders the colors that are bright enough to be blured to the color attachment to this FBO. we can then use that in the blur
 
 			////////////////////////////////////////////////////////
 			// blur pass
 			////////////////////////////////////////////////////////
 			bool horizontalBlur = false;
 			bool firstIteration = true;
 			int numberBlurPasses = 10; //probably need to keep this as an even number
 			bloomShader->use();
 			bloomShader->setUniform1i("image", 0); //set active texture unit for sampler2d
 			for (int blurPass = 0; blurPass < numberBlurPasses; ++blurPass)
 			{
 				bloomShader->setUniform1i("horizontalBlur", horizontalBlur);
 				ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_pingPong[horizontalBlur]));
 
 				ec(glActiveTexture(GL_TEXTURE0));
 				//use horizontal status as if it were an index into ping-pong since it will only ever be 0 or 1 (false or true)
 				//take brightness output if first iteration, otherwise, take the color attachment of the other FBO
 				ec(glBindTexture(GL_TEXTURE_2D, firstIteration ? fbo_attachment_hdrExtractionColor : pingpongColorBuffers[!horizontalBlur]));
 
 				ec(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
 				ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
 
 				//draw quad
 				ndcQuad->render();
 
 				//prepare state for next iteration
 				horizontalBlur = !horizontalBlur;
 				firstIteration = false;
 			}
 
 			////////////////////////////////////////////////////////
 			// prepare blur image to be added to final HDR image color
 			////////////////////////////////////////////////////////
 
 			ec(glBindFramebuffer(GL_FRAMEBUFFER, bMultisampleEnabled ? fbo_multisample : 0));
 
 			//bind textures
 			ec(glActiveTexture(GL_TEXTURE0));
 			ec(glBindTexture(GL_TEXTURE_2D, fbo_attachment_color_tex)); //we're going to add to this texture
 			//ec(glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horrizontalBlur])); //DEBUG: visualize output of blur
 
 			ec(glActiveTexture(GL_TEXTURE1));
 			ec(glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[!horizontalBlur])); //get last rendered colorbuffer, !horrizontalBuffer is the index of last buffer
 
 			//configure shader for rendering HDR with bloom
 			toneMappingShader->use();
 			toneMappingShader->setUniform1i("bEnableBloom", bEnableBloom);
 			toneMappingShader->setUniform1i("gaussianBlur", 1);
 		}
 		else
 		{
 			toneMappingShader->use();
 			toneMappingShader->setUniform1i("bEnableBloom", false);
 		}
 
 		////////////////////////////////////////////////////////
 		// tone mapping
 		////////////////////////////////////////////////////////
 		if (toneMappingShader)
 		{
 			toneMappingShader->use();
 			toneMappingShader->setUniform1i("renderTexture", 0);
 			toneMappingShader->setUniform1i("bEnableHDR", bEnableHDR); //doing this everyframe as we may not be able to set uniforms at shader construction, should probably add that featuer to shaders (cache uniforms and renable on use)
 			ndcQuad->render();
 		}
 
 		ec(glEnable(GL_DEPTH_TEST));
 		if (bool bCopyDepthInformation = true)
 		{
 			ec(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_hdr));
 			ec(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bMultisampleEnabled ? fbo_multisample : 0));
 			ec(glBlitFramebuffer(0, 0, fb_width, fb_height, 0, 0, fb_width, fb_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST));
 		}
 	}
 
	void ForwardRenderingStateMachine::stage_MSAA()
	{
		if (fbo_hdr && bMultisampleEnabled)
		{
			//render the MSAA output back to framebuffer, but using MSAA
			ec(glBindFramebuffer(GL_FRAMEBUFFER, 0));
			ec(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
			ec(glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
			ec(glDisable(GL_DEPTH_TEST)); //don't use depth when rendering back
			//glDisable(GL_STENCIL_TEST); //don't use stencil when rendering back

			MSAA_Shader->use();
			MSAA_Shader->setUniform1i("screencapture", 0);
			MSAA_Shader->setUniform1i("viewport_width", fb_width);
			MSAA_Shader->setUniform1i("viewport_height", fb_height); //#TODO use fragment shader screen coords instead of uniforms

			ec(glActiveTexture(GL_TEXTURE0));
			ec(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fbo_multisample_color_attachment));

			//render the previous buffers to the default buffer, using MSAA to antialias
			ndcQuad->render();
			ec(glEnable(GL_DEPTH_TEST)); //restore depth test
		}
	}

	void ForwardRenderingStateMachine::setUseMultiSample(bool bEnableMultiSample)
	{
		bMultisampleEnabled = bEnableMultiSample;
		//framebuffer_delete();
		//framebuffer_allocate();
	}

	void ForwardRenderingStateMachine::postConstruct()
 	{
 		Parent::postConstruct();
 
 		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
 		windowSystem.onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &ForwardRenderingStateMachine::handlePrimaryWindowChanging);
 		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
 		{
 			handlePrimaryWindowChanging(nullptr, primaryWindow);
 		}
 
 		ndcQuad = new_sp<NdcQuad>();
 		hdrColorExtractionShader = new_sp<Shader>(postProcessForwardShader_default_vs, hdrExtraction_fs, false);
 		bloomShader = new_sp<Shader>(postProcessForwardShader_default_vs, bloomShader_gausblur_fs, false);
 		toneMappingShader = new_sp<Shader>(postProcessForwardShader_default_vs, toneMappingForwardShader_default_fs, false);
		MSAA_Shader = new_sp<Shader>(MSAA_Offscreen_vs, MSAA_Offscreen_fs, false);
 	}
 
 	void ForwardRenderingStateMachine::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
 	{
 		//#todo #nextengine if we only ever have a single primary window... then why not put the framebuffer resizing events on the window system?
 		if (old_window)
 		{
 			old_window->framebufferSizeChanged.removeWeak(sp_this(), &ForwardRenderingStateMachine::handleFramebufferResized);
 		}
 
 		if (new_window)
 		{
 			new_window->framebufferSizeChanged.addWeakObj(sp_this(), &ForwardRenderingStateMachine::handleFramebufferResized);
 			std::pair<int, int> framebufferSize = new_window->getFramebufferSize();
 			handleFramebufferResized(framebufferSize.first, framebufferSize.second); //this may not be safe if window isn't initialized, but probably should be
 		}
 	}
 
 	void ForwardRenderingStateMachine::handleFramebufferResized(int width, int height)
 	{
 		fb_height = height;
 		fb_width = width;
 
 		if (hasAcquiredResources())
 		{
 			framebuffer_delete();
 			framebuffer_allocate();
 		}
 	}
 
 	void ForwardRenderingStateMachine::onAcquireGPUResources()
 	{
 		framebuffer_allocate();
 	}
 
 
 	void ForwardRenderingStateMachine::onReleaseGPUResources()
 	{
 		framebuffer_delete();
 	}
 
 	void ForwardRenderingStateMachine::framebuffer_allocate()
 	{
 		if (fbo_hdr)
 		{
 			STOP_DEBUGGER_HERE(); //there's still a framebuffer built! systme should not be calling this unless it has made sure framebuffer cleared up first, perhaps refactor error
 			framebuffer_delete();
 		}
 
 		if (fb_width <= 0 || fb_height <= 0)
 		{
 			//window resized to framebuffer height we can't allocate for
 			//STOP_DEBUGGER_HERE(); //this happens when minized, don't trip here
 			return;
 		}
 
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		// set up framebuffers for capturing HDR data
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		ec(glGenFramebuffers(1, &fbo_hdr));
 		ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_hdr));
 
 		ec(glGenTextures(1, &fbo_attachment_color_tex));
 		ec(glBindTexture(GL_TEXTURE_2D, fbo_attachment_color_tex));
 		ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, fb_width, fb_height, 0, GL_RGBA, GL_FLOAT, nullptr));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
 		ec(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_attachment_color_tex, 0));
 
 		ec(glGenRenderbuffers(1, &fbo_attachment_depth_rbo));
 		ec(glBindRenderbuffer(GL_RENDERBUFFER, fbo_attachment_depth_rbo));
 		ec(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width, fb_height));
 		ec(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo_attachment_depth_rbo));
 
 		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
 		{
 			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
 			logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating framebuffer %x", error);
 		}
 
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		// set up frame buffers for extracting colors above threshold
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		ec(glGenFramebuffers(1, &fbo_hdrThresholdExtraction));
 		ec(glGenTextures(1, &fbo_attachment_hdrExtractionColor));
 		ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_hdrThresholdExtraction));
 
 		ec(glBindTexture(GL_TEXTURE_2D, fbo_attachment_hdrExtractionColor));
 		ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fb_width, fb_height, 0, GL_RGB, GL_FLOAT, nullptr));
 
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
 		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
 
 		ec(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_attachment_hdrExtractionColor, 0));
 
 		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
 		{
 			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
 			logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating ping pong framebuffers %x", error);
 		}
 
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		//pingpong frame buffers
 		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 		ec(glGenFramebuffers(2, fbo_pingPong));
 		ec(glGenTextures(2, pingpongColorBuffers));
 		for (int buffer = 0; buffer < 2; buffer++)
 		{
 			ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_pingPong[buffer]));
 
 			ec(glBindTexture(GL_TEXTURE_2D, pingpongColorBuffers[buffer]));
 			ec(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fb_width, fb_height, 0, GL_RGB, GL_FLOAT, nullptr));
 
 			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
 			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
 			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
 			ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
 
 			ec(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorBuffers[buffer], 0));
 
 			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
 			{
 				GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
 				logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating ping pong framebuffers %x", error);
 			}
 		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// msaa buffers
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		ec(glGenFramebuffers(1, &fbo_multisample));
		ec(glBindFramebuffer(GL_FRAMEBUFFER, fbo_multisample)); //bind both read/write to the target framebuffer

		ec(glGenTextures(1, &fbo_multisample_color_attachment));
		ec(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fbo_multisample_color_attachment));
		ec(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, fb_width, fb_height, GL_TRUE));
		ec(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0));
		ec(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, fbo_multisample_color_attachment, 0));

		ec(glGenRenderbuffers(1, &fbo_multisample_depthstencil_rbo));
		ec(glBindRenderbuffer(GL_RENDERBUFFER, fbo_multisample_depthstencil_rbo));
		ec(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, fb_width, fb_height));
		ec(glBindRenderbuffer(GL_RENDERBUFFER, 0)); //unbind the render buffer
		ec(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo_multisample_depthstencil_rbo));
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLuint error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "failure creating msaa framebuffer %x", error);
		}
		ec(glBindFramebuffer(GL_FRAMEBUFFER, 0)); //bind to default frame buffer

 		ec(glBindRenderbuffer(GL_RENDERBUFFER, 0));
 		ec(glBindFramebuffer(GL_FRAMEBUFFER, 0));
 
 	}
 
 	void ForwardRenderingStateMachine::framebuffer_delete()
 	{
 		ec(glDeleteFramebuffers(1, &fbo_hdr));
 		ec(glDeleteTextures(1, &fbo_attachment_color_tex));
 		ec(glDeleteRenderbuffers(1, &fbo_attachment_depth_rbo));
 
 		//delete the HDR threshold extraction resources (ie the thing that gets the light that bloom should blur)
 		ec(glGenFramebuffers(1, &fbo_hdrThresholdExtraction));
 		ec(glGenTextures(1, &fbo_attachment_hdrExtractionColor));
 
 		//ping pong fb's for effects like bloom
 		ec(glDeleteFramebuffers(2, fbo_pingPong));
 		ec(glDeleteTextures(2, pingpongColorBuffers));

		//msaa extra buffer
		ec(glDeleteFramebuffers(1, &fbo_multisample));
		ec(glDeleteTextures(1, &fbo_multisample_color_attachment));
		ec(glDeleteRenderbuffers(1, &fbo_multisample_depthstencil_rbo));
 
 		//signal that framebuffer is deleted by making it zero
 		fbo_hdr = 0;
 
 	}
 
 
 }
 
 
