#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include <memory>


namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fast weak pointer. A weak pointer that bypasses the slow locking process. 
	//		Warning: this can be very unsafe. It is extremely unsafe across threads. Also unsafe in single thread.
	//			Pointers may be deleted out from under you. consider the single threaded example.
	//					fwp<TargetType> target = ...;
	//					if (target)
	//					{
	//						fireAtTarget(target); //destroys target, frees memory pointer
	//						target->getHealth(); //access violation, target was just destroyed!
	//					}
	//			Above, the pointer is cleared while still executing code within the if-statement that checked if pointer is valid.
	//
	//		-Find reference on fwp for examples in test files.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class FastWeakPointer
	{
	public:
		FastWeakPointer() : weakptr(), rawPtr(nullptr) {}

		template<typename Y>
		FastWeakPointer(const sp<Y>& copySP)
		{
			weakptr = copySP;
			rawPtr = copySP ? copySP.get() : nullptr;
		}
		template<typename Y>
		FastWeakPointer(const wp<Y>& copyWP)
		{
			weakptr = copyWP;
			rawPtr = !weakptr.expired() ? weakptr.lock().get() : nullptr;
		}

		FastWeakPointer(std::nullptr_t nullptrvalue)
		{
			weakptr = sp<T>(nullptr);
			rawPtr = nullptr;
		}
		FastWeakPointer(const FastWeakPointer& copy) = default;
		FastWeakPointer(FastWeakPointer&& move) = default;
		FastWeakPointer& operator=(const FastWeakPointer& copy) = default;
		FastWeakPointer& operator=(FastWeakPointer&& move) = default;

	public:
		inline operator bool() const noexcept { return !weakptr.expired(); }
		inline operator wp<T>() const noexcept { return weakptr; }
		T* operator->() const noexcept { return rawPtr; }
		T& operator*() const noexcept { return *rawPtr; }
	private:
		wp<T> weakptr;
		T* rawPtr;
	};

	////////////////////////////////////////////////////////
	// fast weak pointer alias
	////////////////////////////////////////////////////////
	template<typename T>
	using fwp = FastWeakPointer<T>;
}

