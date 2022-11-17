#pragma once

#include<glad/glad.h> 
#include <vector>

#include "../../../GameFramework/SAGameEntity.h"
#include "../../../Rendering/SAGPUResource.h"
#include "../../../GameFramework/Interfaces/SATickable.h"
#include "../../../Tools/DataStructures/SATransform.h"
#include "../../../Tools/DataStructures/MultiDelegate.h"
#include <optional>
#include "../../../GameFramework/CurveSystem.h"

namespace SA
{

	constexpr const char* const LaserRNGKey = "LaserUIRNG";

	class Window;
	class Shader;
	class RNG;
	class LevelBase;
	struct GameUIRenderData;

	enum class ELaserOffscreenMode
	{
		TOP, LEFT, RIGHT, BOTTOM
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents a single laser used to render UI elements
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class LaserUIObject : public GameEntity
	{
	public:
		struct InstanceRenderData
		{
			glm::vec3 startPnt;
			glm::vec3 endPnt;
			glm::vec3 color;
		};
	public:
		void animateStartTo(const glm::vec3& startPoint);
		void animateEndTo(const glm::vec3& endPoint);
		void animate(const glm::vec3& startPoint, const glm::vec3& endPoint);
		void update_NoAnimReset(const glm::vec3& startPoint, const glm::vec3& endPoint);
		void updateEnd_NoAnimReset(const glm::vec3& endPoint);
		void updateStart_NoAnimReset(const glm::vec3& startPoint);
		void multiplexedUpdate(bool bResetAnimation, const glm::vec3& startPoint, const glm::vec3& endPoint);
		void randomizeAnimSpeed(float targetAnimDurationSecs = 1.0f, float randomDriftSecs = 0.25f);
		void setAnimDurations(float startSpeedSec, float endSpeedSec);
		void scaleAnimSpeeds(float startScale, float endScale);
		float getRandomAnimTimeOffset(float rangeSecs);
		void forceAnimComplete();
		void setColorImmediate(const glm::vec3& color);
	protected:
		virtual void postConstruct() override;
	private:
		friend class LaserUIPool;
		bool tick(float dt_sec);	
		void LerpToGoalPositions();
		void prepareRender(GameUIRenderData& renderData, InstanceRenderData& outInstanceData);
		void setOffscreenMode(const std::optional<ELaserOffscreenMode>& inOffscreenMode, bool bResetAnimProgress = true);
		void setOffscreenMode(const std::optional<ELaserOffscreenMode>& inOffscreenMode, bool bResetAnimProgress, GameUIRenderData& shared_data);
	public:
		MultiDelegate<> onReachedLocation;
		MultiDelegate<> onReachedDeactivationLocation;
	public:
		struct AnimData
		{
			glm::vec3 begin{ 0.f };
			glm::vec3 end{ 0.f };
			glm::vec3 camToBegin{ 0.f };
			glm::vec3 camToEnd{ 0.f };
			float animDuration = 1.f;
			float curTickTimeSec = 0.f;
		};
		bool bForceCameraRelative = false;
	private:
		AnimData anim_Start;
		AnimData anim_End;
	private:
		sp<RNG> rng = nullptr;
		float startLERPSpeed_sec = 1.0f;
		float endLERPSpeed_sec = 1.0f;

		glm::vec3 startPos;	//actual line position
		glm::vec3 endPos;	//actual line position
		glm::vec3 color{ 1,0,0};
		std::optional<ELaserOffscreenMode> offscreenMode;

		bool bInvalidatedForLevelCleanup = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A pool to request and release LaserUIObjects to give transitions.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class LaserUIPool : public GPUResource, public ITickable
	{
	public:
		static LaserUIPool& get();
		static Curve_highp laserLerpCurve;		//non const to allow changing of defaults
		static glm::vec3 defaultColor;			//non const to allow changing of defaults
		virtual ~LaserUIPool();
		sp<LaserUIObject> requestLaserObject();
		sp<LaserUIObject> requestLaserObject(std::vector<sp<LaserUIObject>>& outContainer); 
		
		void releaseLaser(sp<LaserUIObject>& out); //will null out shared pointer for you as a signal it was released
		void assignRandomOffscreenMode(LaserUIObject& laser);

	protected:
		virtual void postConstruct() override;
	protected:
		virtual void onReleaseGPUResources() override;
		virtual void onAcquireGPUResources() override;
		virtual bool tick(float dt_sec) override;
		void renderGameUI(GameUIRenderData& renderData);
	private:
		void handleGameShutdownStarted();
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleFramebufferResized(int width, int height);
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
	private:
		std::vector<LaserUIObject*> activeLasers;
		std::vector<sp<LaserUIObject>> unclaimedPool;
		std::vector<sp<LaserUIObject>> ownedLaserObjects; /** Actual instantiated laser objects*/
		sp<RNG> rng;
	private:
		std::array<glm::vec4, 4> basisVecsCPU = {
			glm::vec4(1,0,0,1),	//1 needed in w coord to get view translation; see debuglineshader notes.
			glm::vec4(0,1,0,1),
			glm::vec4(0,0,1,1),
			glm::vec4(0,0,0,1)
		};
		std::vector<glm::mat4> instance_ShearMatrices;
	private:
		sp<Shader> laserShader;
		GLuint vao = 0;
		GLuint vbo_pos = 0;
		GLuint vbo_instance_shearModels = 0;	//use shear matrices to transform basis vectors
	};
}