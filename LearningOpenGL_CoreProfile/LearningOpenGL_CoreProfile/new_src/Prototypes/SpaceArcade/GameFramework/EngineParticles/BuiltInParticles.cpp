#include "BuiltInParticles.h"
#include "..\SAParticleSystem.h"
#include "..\..\Tools\ModelLoading\SAModel.h"
#include "..\..\Tools\Geometry\SimpleShapes.h"
#include "..\SAGameBase.h"
#include "..\SALog.h"
#include "..\SAAssetSystem.h"


namespace SA
{
	using glm::vec3; using glm::vec4; using glm::normalize; using glm::cross; using glm::dot;
	using glm::ivec3;

	namespace ShieldEffect
	{
		static char const* const ShieldShaderVS_src = R"(
			#version 330 core
			layout (location = 0) in vec3 position;			
			layout (location = 1) in vec3 normal;	
			layout (location = 2) in vec2 uv;
            //layout (location = 3) in vec3 tangent;
            //layout (location = 4) in vec3 bitangent; //may can get 1 more attribute back by calculating this cross(normal, tangent);
				//layout (location = 5) in vec3 reserved;
				//layout (location = 6) in vec3 reserved;
			layout (location = 7) in vec4 effectData1; //x=timeAlive, y=fractionComplete
			layout (location = 8) in mat4 model; //consumes attribute locations 8,9,10,11
				//layout (location = 12) in vec3 reserved;
				//layout (location = 13) in vec3 reserved;
				//layout (location = 14) in vec3 reserved;
				//layout (location = 15) in vec3 reserved;
				
			uniform mat4 projection_view;
			uniform vec3 camPos;
			uniform float normalOffsetDist = 0.01f;

			//out vec3 fragNormal;
			out vec3 fragPosition;
			out vec2 uvCoords;
			out float timeAlive;
			out float fractionComplete;

			void main(){
				//offset the local space vert position by the normal, this is so the effect will surround the actual model.
				vec4 offsetPosition_ls = vec4(position + (normalOffsetDist * normalize(normal)), 1);

				gl_Position = projection_view * model * offsetPosition_ls;
				fragPosition = vec3(model * offsetPosition_ls);

				timeAlive = effectData1.x;
				float effectEndTime = effectData1.y;
				fractionComplete = timeAlive / effectEndTime;

				//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
				//fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

				uvCoords = uv;
			}	
		)";

		static char const* const ShieldShaderFS_src = R"(
			#version 330 core

			out vec4 fragmentColor;

			uniform vec3 camPos;
			uniform vec3 shieldColor;
			uniform sampler2D tessellateTex;

			//in vec3 fragNormal;
			in vec3 fragPosition;
			in vec2 uvCoords;
			in float timeAlive;
			in float fractionComplete;

			void main(){

				////////////////////////////////////////////
				//get a grayscale tessellated pattern
				////////////////////////////////////////////
				vec2 preventAlignmentOffset = vec2(0.1, 0.2);
				float medMove = 0.5f *fractionComplete;
				float smallMove = 0.5f *fractionComplete;

				vec2 baseEffectUV = uvCoords * vec2(5,5);
				vec2 mediumTessUV_UR = uvCoords * vec2(10,10) + vec2(medMove, medMove) + preventAlignmentOffset;
				vec2 mediumTessUV_UL = uvCoords * vec2(10,10) + vec2(medMove, -medMove);

				vec2 smallTessUV_UR = uvCoords * vec2(20,20) + vec2(smallMove, smallMove) + preventAlignmentOffset;
				vec2 smallTessUV_UL = uvCoords * vec2(20,20) + vec2(smallMove, -smallMove);

				//invert textures so black is now white and white is black (1-color); 
				vec4 small_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
				vec4 small_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, smallTessUV_UR);
				vec4 med_ur = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UR);
				vec4 med_ul = vec4(vec3(1.f),0.f) - texture(tessellateTex, mediumTessUV_UL);

				vec4 grayScale = 0.25f*small_ur + 0.25f*small_ul + 0.25f*med_ur + 0.25f*med_ur;

				////////////////////////////////////////////
				// filter out some color
				////////////////////////////////////////////
				float cutoff = max(fractionComplete, 0.5f); //starts to disappear once fractionComplete takes over
				if(grayScale.r < cutoff)
				{
					discard;
				}
				
				////////////////////////////////////////////
				// colorize grayscale
				////////////////////////////////////////////
				fragmentColor = grayScale * vec4(shieldColor, 1.f);
				
			}
		)";

		sp<ParticleConfig> ParticleCache::getEffect(const sp<Model3D>& model, const glm::vec3& color)
		{
			sp<ShieldParticleConfig> particle;
			if (!model) { return particle; }

			vec3 normalizedColor = glm::clamp(color, vec3(0, 0, 0), vec3(1, 1, 1));
			normalizedColor *= vec3(255);

			byte colorBytes[3];
			colorBytes[0] = static_cast<byte>(normalizedColor.r);
			colorBytes[1] = static_cast<byte>(normalizedColor.g);
			colorBytes[2] = static_cast<byte>(normalizedColor.b);

			Model3D* modelPtr = model.get();
			size_t hash = std::hash<Model3D*>{}(modelPtr); //instantiate struct, then use its operator()

			uint32_t packedColors = 0;
			packedColors |= colorBytes[0];
			packedColors <<= sizeof(byte);
			packedColors |= colorBytes[1];
			packedColors <<= sizeof(byte);
			packedColors |= colorBytes[2];
			packedColors <<= sizeof(byte);

			hash ^= std::hash<uint32_t>{}(packedColors);

			using UMMIter = decltype(modelToParticleHashMap)::iterator;
			std::pair<UMMIter, UMMIter> bucketRange = modelToParticleHashMap.equal_range(hash); //gets start_end iter pair

			for (UMMIter bucketIter = bucketRange.first; bucketIter != bucketRange.second; ++bucketIter)
			{
				sp<ShieldParticleConfig>& shieldConfig = bucketIter->second;
				if (
					shieldConfig->model.get() == modelPtr
					&& shieldConfig->colorBytes[0] == colorBytes[0]
					&& shieldConfig->colorBytes[1] == colorBytes[1]
					&& shieldConfig->colorBytes[2] == colorBytes[2]
					)
				{
					particle = shieldConfig;
					break;
				}
			}

			//if we didn't find a particle, then we need to create one!
			if (!particle)
			{
				//load texture
				GLuint tesselatedTexture = 0;
				if (!GameBase::get().getAssetSystem().loadTexture("GameData/engine_assets/TessellatedShapeRadials.png", tesselatedTexture))
				{
					log("shield particle effect", LogLevel::LOG_ERROR, "Failed to get texture!");
				}

				//create effect
				std::vector<sp<Particle::Effect>> effects;

				sp<Particle::Effect> shieldModelEffect = new_sp<Particle::Effect>();
				{
					sp<Shader> shieldShader= new_sp<Shader>(ShieldShaderVS_src, ShieldShaderFS_src, false);
					shieldModelEffect->shader = shieldShader;
					shieldModelEffect->mesh = new_sp<Model3DWrapper>(model, shieldShader);

					//material : 
					{
						shieldModelEffect->materials.emplace_back( tesselatedTexture, "tessellateTex");
					}
					shieldModelEffect->keyFrameChains.emplace_back();
					Particle::KeyFrameChain& scaleKFC = shieldModelEffect->keyFrameChains.back();

					//need to animate something to give particle lifetime
					scaleKFC.vec3KeyFrames.emplace_back();
					Particle::KeyFrame<vec3>& scaleFrame = scaleKFC.vec3KeyFrames.back();
					scaleFrame.startValue = vec3(1.f);
					scaleFrame.endValue = vec3(1.f);
					scaleFrame.durationSec = 1.0f;
					scaleFrame.dataIdx = MutableEffectData::SCALE_VEC3_IDX;

					shieldModelEffect->vec3Uniforms.emplace_back("shieldColor", color);

					effects.push_back(shieldModelEffect);
				}
				//create particle and set up its hash data.
				particle = new_sp<ShieldParticleConfig>(std::move(effects));
				
				particle->model = model;
				std::memcpy(particle->colorBytes, colorBytes, 3); 

				modelToParticleHashMap.emplace(hash, particle); 
			}

			return particle;
		}

		void ParticleCache::resetCache()
		{
			modelToParticleHashMap.clear();
		}

	}
}

