#pragma once
#include <string>
#include "../SAGPUResource.h"
#include "../../Tools/DataStructures/SATransform.h"
#include <optional>

namespace SA
{
	class Texture_2D : public GPUResource
	{
	public:
		Texture_2D(const std::string& filePath) : filePath(filePath) {};
		Texture_2D(glm::vec3 solidColor) : solidColor(solidColor) {};
	public:
		unsigned int getTextureId() const { return textureId; }
		void bindTexture(unsigned int textureSlot);
	private:
		virtual void onAcquireGPUResources() override;
		virtual void onReleaseGPUResources() override;
	private:
		const std::string filePath;
		const std::optional<glm::vec3> solidColor = std::nullopt;
		unsigned int textureId;
		bool bLoadSuccess = false;
	};
}

