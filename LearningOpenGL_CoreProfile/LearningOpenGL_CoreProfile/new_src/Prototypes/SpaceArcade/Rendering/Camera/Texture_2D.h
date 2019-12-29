#pragma once
#include <string>
#include "../SAGPUResource.h"
namespace SA
{
	class Texture_2D : public GPUResource
	{
	public:
		Texture_2D(const std::string& filePath) : filePath(filePath) {};
	public:
		unsigned int getTextureId() const { return textureId; }
		void bindTexture(unsigned int textureSlot);
	private:
		virtual void onAcquireGPUResources() override;
		virtual void onReleaseGPUResources() override;
	private:
		const std::string filePath;
		unsigned int textureId;
		bool bLoadSuccess = false;
	};
}

