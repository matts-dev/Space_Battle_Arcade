#pragma once
#include "../../GameFramework/SAGameEntity.h"

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

}