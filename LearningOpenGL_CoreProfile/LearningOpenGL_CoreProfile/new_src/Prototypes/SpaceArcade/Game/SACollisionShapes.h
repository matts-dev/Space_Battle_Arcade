#pragma once
#include "..\GameFramework\SAGameEntity.h"
#include <vector>

namespace SAT
{
	//forward declarations
	class Model;
	class Shape;
}

namespace SA
{
	enum class ECollisionShape : int
	{
		CUBE, POLYCAPSULE, WEDGE, PYRAMID, ICOSPHERE, UVSPHERE
	};
	const char* const shapeToStr(ECollisionShape value);

	class CollisionShapeFactory : public GameEntity
	{
	public:
		CollisionShapeFactory();

	public:
		sp<SAT::Shape> generateShape(ECollisionShape shape) const;

		/* For debug rendering*/
		sp<const SAT::Model> getModelForShape(ECollisionShape shape) const;
	};

}