#pragma once
#include "../SAGameEntity.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Provides a handle to an asset in a way that encapsulates data so user must always request direct reference
	// to the data.
	//
	// This is because an asset could be unloaded and a user holding shared pointer would keep asset in memory.
	// If we just provide weak pointers, then programmers will be tempted to cache it and hold a shared pointer to it
	//
	//Providing this handle lets them hold the handle and request asset every time they want to use it. 
	// protecting them accidentally causing essentially memory leaks
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class AssetHandle
	{
	public:
		T* getAsset();
		const T* getAsset() const;
	public:
		AssetHandle(wp<T> assetData) : weakDataPtr(assetData) {};
		AssetHandle(sp<T> assetData) : weakDataPtr(assetData) {};
		AssetHandle(std::nullptr_t){};
		AssetHandle(const AssetHandle& copy) = default;
		AssetHandle(AssetHandle&& move) = default;
		AssetHandle& operator= (const AssetHandle& copy) = default;
		AssetHandle& operator= (AssetHandle&& move) = default;
	private:
		wp<T> weakDataPtr;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Template Implementation
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	const T* SA::AssetHandle<T>::getAsset() const
	{
		if (!this->weakDataPtr.expired())
		{
			if (auto sp_data = this->weakDataPtr.lock())
			{
				return sp_data.get();
			}
		}
		return nullptr;
	}

	template<typename T>
	T* SA::AssetHandle<T>::getAsset()
	{
		if (!this->weakDataPtr.expired())
		{
			if (auto sp_data = this->weakDataPtr.lock())
			{
				return sp_data.get();
			}
		}
		return nullptr;
	}

}