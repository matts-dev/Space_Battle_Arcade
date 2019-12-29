#include "Texture_2D.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SALog.h"
#include "../OpenGLHelpers.h"
namespace SA
{

	void Texture_2D::bindTexture(unsigned int textureSlot)
	{
		//GL_TEXTURE0 is an example of a textureSlot

		if (hasAcquiredResources() && bLoadSuccess)
		{
			ec(glActiveTexture(textureSlot));
			ec(glBindTexture(GL_TEXTURE_2D, textureId));
		}
	}

	void Texture_2D::onAcquireGPUResources()
	{
		static AssetSystem& assetSystem = GameBase::get().getAssetSystem();
		if (assetSystem.loadTexture(filePath.c_str(), textureId))
		{
			bLoadSuccess = true;
		}
		else
		{
			bLoadSuccess = false;
			log("texture load fail", LogLevel::LOG_ERROR, filePath.c_str());
		}
	}

	void Texture_2D::onReleaseGPUResources()
	{
		static AssetSystem& assetSystem = GameBase::get().getAssetSystem();
		//#TODO asset system doesn't seem to clean up itself, nor does it provide a way to clean up other than shutting down
	}

}

