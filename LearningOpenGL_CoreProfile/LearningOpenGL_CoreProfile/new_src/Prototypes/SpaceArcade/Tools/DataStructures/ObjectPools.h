#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include <optional>

namespace SA
{
	template<typename T>
	class SP_SimpleObjectPool
	{
	public:
		sp<T> getInstance()
		{
			if (pool.size() > 0)
			{
				sp<T> instance = pool[pool.size() - 1];
				pool.pop_back();
				return instance;
			}
			else
			{
				return new_sp<T>();
			}
		}

		void releaseInstance(const sp<T>& objectToReclaim)
		{
			if (objectToReclaim)
			{
				pool.push_back(objectToReclaim);
			}
			else
			{
				assert(false); //releasing nullptr to object pool;
			}
		}

	private:
		std::vector<sp<T>> pool;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// primitve pool
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class Pool
	{
	public:
		std::optional<T> getInstance()
		{
			if (pool.size() > 0)
			{
				T instance = pool.back();
				pool.pop_back();
				return instance;
			}
			else
			{
				return std::nullopt;
			}
		}

		void releaseInstance(const T& object)
		{
			pool.push_back(object);
		}

		void reserve(size_t size)
		{
			pool.reserve(size);
		}

	private:
		std::vector<T> pool;
	};

}