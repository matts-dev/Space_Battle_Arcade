#pragma once

#include "Game/AssetConfigs/SAConfigBase.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace SA
{
	class Model3D;
	class Window;

	class ProjectileConfig final : public ConfigBase
	{
		friend class ProjectileTweakerWidget;

	public:
		virtual std::string getRepresentativeFilePath() override;
		
		//accessors
		const sp<Model3D>& getModel() const { return model; }
		glm::vec3 getAABBsize() const { return aabbSize; }
		float getSpeed() const { return speed; }
		float getLifetimeSecs() const { return lifetimeSecs; }
		glm::vec3 getColor() const {return color;}

	protected:
		virtual void postConstruct() override;
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;

	private:
		void handleOnWindowAquiredOpenglContext(const sp<Window>& window);

	private: //non-serialized properties
		sp<Model3D> model;
		glm::vec3 aabbSize;

	private: //serialized properties
		float speed;
		float lifetimeSecs;
		glm::vec3 color{1,1,1};
	};
}