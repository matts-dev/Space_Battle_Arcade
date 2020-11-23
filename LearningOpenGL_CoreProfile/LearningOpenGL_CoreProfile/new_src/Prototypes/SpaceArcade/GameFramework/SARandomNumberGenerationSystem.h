#pragma once
#include <string>
#include <unordered_map>
#include <random>
#include <cstdint>

#include "SASystemBase.h"
#include "SAGameEntity.h"

#define SA_RNG_USE_TIME 1 //#todo #shipping

namespace SA
{
	class RNG;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Random number generator system 
	//
	//		RNG system creates named random number generators.
	//
	//		Design considerations
	//			-having owned random number generators allows systems to have number generators that are independent.
	//			-having independent RNGs helps create a predicable scenario for debugging.
	//				-for example, when debugging a character's random behavior, we can reduce the number of characters in the world to 1
	//				-but leave the random generation of the world untouched. Because they are using two independent RNGs, we can observe the same sequence on the character every time.
	//			-When execution time influences when we requests for random numbers, then enter the land of unreproducible behavior.
	//				*having the ability to isolate RNGs from each other can allow system design that controls RNG's associated with system timing
	//					-consider two of timers A (0.15sec) and B (0.1sec) that share an RNG, at the end of these timers a random number is requested and a chain of small deferred RNG requests are made.
	//					-scenario 1: system delta time is 0.5sec. Both Timers fire. A gets RNG "5" and B gets RNG"6".
	//					-scenario 2: system delta time is 0.12sec; B fires and gets "5", on next  tick A fires and gets "6"
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class RNGSystem : public SystemBase
	{
	public:
		/*Random number generation in response to system runtime events can create unpredictable behavior; for RNGs in that domain 
		use this creation method to isolate them from the predictable named generators.*/
		sp<RNG> getTimeInfluencedRNG();
		sp<RNG> getNamedRNG(const std::string rngName);
		sp<RNG> getSeededRNG(uint32_t seed);

	protected:
		virtual void postConstruct();
	private:
		sp<RNG> createNewRNG(sp<RNG>& seedSrcRNG);
	private:
		sp<RNG> rootNamedRNG;		//generator that spawns seeds for named generators
		sp<RNG> rootTimeInfluencedRNG; //used to spawn seeds new RNGs that
		std::unordered_map <std::string, sp<RNG>> namedGenerators;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Random number generator.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class RNG : public RemoveCopies, public RemoveMoves
	{
	private:
		friend class RNGSystem;

		/*random number generation must start from random number generation system to help with systemic predictability*/
		RNG(std::initializer_list<uint32_t> seedSequenceInitList)
			: seed(seedSequenceInitList)
		{
			rng_eng = std::mt19937(seed);
		}

	public:
		template<typename T = int>
		T getInt(T lowerInclusive = 0, T upperInclusive = std::numeric_limits<T>::max())
		{
			static_assert(std::is_integral<T>::value, "must provide integer type");
			std::uniform_int_distribution<T> distribution(lowerInclusive, upperInclusive);
			return distribution(rng_eng);
		}

		/*Notice the maximums are not numeric_limits max. Microsoft's checks cause as assert with max values, it appears to be due to float imprecision at large numbers*/
		template<typename T = float>
		T getFloat(T lowerInclusive = -std::numeric_limits<T>::max() / 2, T upperExclusive = std::numeric_limits<T>::max() / 2)
		{
			static_assert(std::is_floating_point<T>::value, "must provide floating point type (eg float, double)");
			//#optimize: profiling with large numbers shows re-creating the distribution is **very slightly** slower than sharing a distribution. (see cpp-learning-repro RandomDistributionCreation under profiling)
			//  alternative: create shared distribution of [0,1). Then do [0,1]*|desired_range| -|range_portion_below_zero| (or something of that nature (there's lot of edge cases)
			//  alternatives will need profiling as the floating point arithmetic overhead may be worse than just creating a new uniform distribution
			std::uniform_real_distribution<T> distribution(lowerInclusive, upperExclusive);
			return distribution(rng_eng);
		}

	private:
		std::seed_seq seed;
		std::mt19937 rng_eng;
	};
}