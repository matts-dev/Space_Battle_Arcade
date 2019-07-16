#include "SAProjectileConfig.h"
#include "..\SpaceArcade.h"
#include "..\..\GameFramework\SAAssetSystem.h"
#include "..\..\Tools\ModelLoading\SAModel.h"
#include "..\..\GameFramework\SAWindowSystem.h"
#include "..\..\Rendering\SAWindow.h"
#include "..\..\GameFramework\SALog.h"

namespace SA
{
	std::string ProjectileConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/ProjectileConfigs/") + name + std::string(".json");
	}

	void ProjectileConfig::onSerialize(json& outData)
	{
		json projectileData =
		{
			{"speed", speed},
			{"lifetimeSecs", lifetimeSecs},
			{"color", {color.r, color.g, color.b}}
		};
		outData.push_back({ "ProjectileConfig", projectileData });
	}

	void ProjectileConfig::onDeserialize(const json& inData)
	{
		if (!inData.is_null() && inData.contains("ProjectileConfig"))
		{
			const json& projectileData = inData["ProjectileConfig"];
			if (!projectileData.is_null())
			{
				if (projectileData.contains("speed") && !projectileData["speed"].is_null())
				{
					speed = projectileData["speed"];
				}
				if (projectileData.contains("lifetimeSecs") && !projectileData["lifetimeSecs"].is_null())
				{
					lifetimeSecs = projectileData["lifetimeSecs"];
				}
				if (projectileData.contains("color") && !projectileData["color"].is_null())
				{
					const json& loadedColor = projectileData["color"];
					color.r = loadedColor[0];
					color.g = loadedColor[1];
					color.b = loadedColor[2];
				}
			}
		}
	}

	void ProjectileConfig::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		WindowSystem& windowSystem = game.getWindowSystem();

		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &ProjectileConfig::handleOnWindowAquiredOpenglContext);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleOnWindowAquiredOpenglContext(primaryWindow);
		}
	}

	void ProjectileConfig::handleOnWindowAquiredOpenglContext(const sp<Window>& window)
	{
		if (!model)
		{
			//load model
			SpaceArcade& game = SpaceArcade::get();
			model = game.getAssetSystem().loadModel(game.URLs.laserURL);
			if (model)
			{
				std::tuple<glm::vec3, glm::vec3> modelAABB = model->getAABB();

				const glm::vec3& aabbMin = std::get<0>(modelAABB);
				const glm::vec3& aabbMax = std::get<1>(modelAABB);
				aabbSize = aabbMax - aabbMin;
			}
			else
			{
				log("ProjectileConfig", LogLevel::LOG_ERROR, "Failed to load model for projectile");
			}
		}
	}

}

