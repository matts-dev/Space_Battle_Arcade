#pragma once
#include "SAWorldEntity.h"
#include "..\Tools\DataStructures\SATransform.h"
#include "..\Tools\ModelLoading\SAModel.h"

namespace SA
{
	class RenderModelEntity : public WorldEntity
	{
	public:
		RenderModelEntity(const sp<Model3D>& inModel, const Transform& spawnTransform = Transform{})
			: WorldEntity(spawnTransform),
			model(inModel),
			constView(inModel)
		{}
		const sp<const Model3D>& getModel() const { return constView; }
	private:
		sp<Model3D> model;
		sp<const Model3D> constView;
	
	};


	/** This is done separately so most classes implementing RenderModelEntity must provide collision 
		implementations that either opt-out or opt-in to collision*/
	class RenderModelEntity_NoCollision final : public RenderModelEntity
	{
	public:
		RenderModelEntity_NoCollision(const sp<Model3D>& inModel, const Transform& spawnTransform = Transform{})
			: RenderModelEntity(inModel, spawnTransform)
		{}

		virtual const sp<const ModelCollisionInfo>& getCollisionInfo() const override;
		virtual bool hasCollisionInfo() const override;
	};
}