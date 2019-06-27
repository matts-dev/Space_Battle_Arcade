#pragma once
#include <string>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "..\GameFramework\SAGameEntity.h"
#include <vector>

namespace SA
{
	enum class ECollisionShape : int
	{
		CUBE, POLYCAPSULE
	};
	const char* const shapeToStr(ECollisionShape value);

	struct CollisionShapeConfig
	{
		int shape;		//avoiding strict alias violation, this is int type rather than enum
		glm::vec3 scale{ 1,1,1 };
		glm::vec3 rotationDegrees{ 0,0,0 };
		glm::vec3 position{ 0,0,0 };
	};

	class SpawnConfig
	{
		friend class ModelConfigurerEditor_Level;
		class PrivateKey
		{
			friend class Mod;
			PrivateKey() {};
		};

	public:
		const std::string& getName() const { return name; }
		const std::string& getModelFilePath() const { return fullModelFilePath; };
		bool isDeletable() const { return bIsDeleteable; }

		std::string serialize();
		void deserialize(const std::string& str);

		std::string getRepresentativeFilePath();

	public:
		static sp<SpawnConfig> load(std::string filePathCopy);
	private:
		void save(); //access restricted, only allow certain classes to save this.

	private: //serialized properities
		std::string name;
		std::string fullModelFilePath;
		bool bIsDeleteable = true;

		glm::vec3 modelScale = glm::vec3(1,1,1);
		glm::vec3 modelRotationDegrees = glm::vec3(0,0,0);
		glm::vec3 modelPosition = glm::vec3(0, 0, 0);

		bool bUseModelAABBTest = true;
		
		std::vector<CollisionShapeConfig> shapes;
		
		//color/material
		//team
	private: //nonserialized properites
		std::string owningModDir;      //do not serialize this, spawn configs should be copy-and-pastable to other mods; set on loading
	};

}
