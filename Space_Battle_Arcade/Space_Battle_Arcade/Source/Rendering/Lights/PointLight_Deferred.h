#pragma once
#include "Tools/RemoveSpecialMemberFunctionUtils.h"
#include "Tools/DataStructures/SATransform.h"
#include "GameFramework/SAGameEntity.h"

namespace SA
{
	class Shader;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A pointer light that gameplay code can own. 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class PointLight_Deferred : public GameEntity, public RemoveCopies, public RemoveMoves
	{
		friend class RenderSystem;
		struct PrivateConstructionKey {};
	public:
		PointLight_Deferred(const PrivateConstructionKey& key) {}
	public: 
		struct UserData
		{
			glm::vec3 position = {};
			glm::vec3 ambientIntensity = {};
			glm::vec3 diffuseIntensity = {};
			glm::vec3 specularIntensity = {};
			bool bActive = false;
			float attenuationConstant = 1.f;
			float attenuationLinear = 0.7f;
			float attenuationQuadratic = 1.8f;
		};
		struct SystemMetaData
		{
			bool bUserDataDirty = true;
			float maxRadius = 1.f;
		};
	public:
		void setPosition(glm::vec3 inPosition) { userData.position = inPosition; }
		inline const UserData& getUserData() const { return userData; }
		inline UserData& getMutableUserData()  //used to update things that require heavy calculation (eg intensity attenuation, etc)
		{
			systemMetaData.bUserDataDirty = true;
			return userData;
		}
		const SystemMetaData& getSystemData() { return systemMetaData; }
		void applyUniforms(Shader& shader);
		void clean();
	public:
		UserData userData = {};
		SystemMetaData systemMetaData = {};
	};

}