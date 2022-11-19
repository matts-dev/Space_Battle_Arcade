#pragma once
#include "SAWorldEntity.h"
#include "Tools/DataStructures/SATransform.h"
#include "Tools/ModelLoading/SAModel.h"

namespace SA
{
	class Shader;

	class RenderModelEntity : public WorldEntity
	{
	public:
		RenderModelEntity(const sp<Model3D>& inModel, const Transform& spawnTransform = Transform{})
			: WorldEntity(spawnTransform),
			model(inModel),
			constView(inModel)
		{}
		const sp<const Model3D>& getModel() const { return constView; }
		virtual void render(Shader& shader);
		virtual void onLevelRender() {};
	protected:
		const sp<Model3D>& getMyModel() const { return model; }
		void replaceModel(const sp<Model3D>& newModel);
	private:
		sp<Model3D> model;
		sp<const Model3D> constView;
	};


}