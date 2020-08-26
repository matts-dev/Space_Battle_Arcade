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
			else{assert(false); /*releasing nullptr to object pool*/}
		}

		void clear() {pool.clear();}
	private:
		std::vector<sp<T>> pool;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// pool for objects where simple construction isn't allowed
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class SP_SimpleObjectPool_RestrictedConstruction
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
			return nullptr;
		}

		void releaseInstance(const sp<T>& objectToReclaim)
		{
			if (objectToReclaim)
			{
				pool.push_back(objectToReclaim);
			}
			else{assert(false); /*releasing nullptr to object pool*/}
		}
		void clear() { pool.clear(); }
	private:
		std::vector<sp<T>> pool;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// primitive pool
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class PrimitivePool
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
		size_t size() const { return pool.size(); }

	private:
		std::vector<T> pool;
	};

}