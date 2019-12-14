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
		operator bool() const noexcept { return !weakptr.expired(); }
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Life time pointer - Shared pointer that automatically releases when an object is destroyed.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//template<typename T>
	//class LifetimePointer
	//{
	//private:
	//	template<class Y>
	//	void entityCompileCheck() const { static_assert(std::is_base_of<SA::GameEntity, Y>::value, "lifetime pointers must point to a game entity."); }
	//	
	//public:
	//	template<typename Y>
	//	LifetimePointer() : sharedPtr(), rawPtr(nullptr)
	//	{
	//		entityCompileCheck<Y>();
	//	}

	//	template<typename Y>
	//	LifetimePointer(const sp<Y>& copySP)
	//	{
	//		entityCompileCheck<Y>();
	//		bindToPtr(copySP)
	//	}

	//	template<typename Y>
	//	LifetimePointer(const wp<Y>& copyWP)
	//	{
	//		entityCompileCheck<Y>();
	//		if (!copyWP.expired())
	//		{
	//			bindToPtr(copyWP.lock());
	//		}
	//	}

	//	LifetimePointer(std::nullptr_t nullptrvalue)
	//	{
	//		entityCompileCheck<T>();
	//		bindToPtr(sp<T>(nullptr));
	//	}

	//	cannot_be_defaults_because_bindings_need_clearing;
	//	LifetimePointer(const LifetimePointer& copy) = default;
	//	LifetimePointer(LifetimePointer&& move) = default;
	//	LifetimePointer& operator=(const LifetimePointer& copy) = default;
	//	LifetimePointer& operator=(LifetimePointer&& move) = default;

	//	care_needed_not_to_release_ptr_if_only_1_refcount___do_next_tick;
	//public:
	//	operator bool() const noexcept { return sharedPtr; }
	//	T* operator->() const noexcept { return sharedPtr.get(); }
	//	T& operator*() const noexcept { return *sharedPtr; }
	//private:
	//	struct EventForwarder : public GameEntity
	//	{
	//		EventForwarder(LifetimePointer<T> owner) : owningLP(owner) {}
	//		void bind(GameEntity& entity)		{ entity.onDestroyedEvent->addStrongObj(sp_this(), &EventForwarder::handleDestroyed); }
	//		void release(GameEntity& entity)	{ entity.onDestroyedEvent->removeStrong(sp_this(), &EventForwarder::handleDestroyed); }
	//		void handleDestroyed()				{ owningLP->notifyDestroyed(); }
	//		LifetimePointer<T>& owningLP;
	//	};

	//	friend EventForwarder;
	//	void notifyDestroyed()
	//	{
	//		eventForwarder.release(*sharedPtr);
	//	}
	//	void bindToPtr(const sp<T>& newPtr)
	//	{
	//		if (!eventForwarder)
	//		{
	//			eventForwarder = new_sp<EventForwarder<T>>(this);
	//		}

	//		if (sharedPtr)
	//		{
	//			eventForwarder->release(sharedPtr);
	//		}

	//		sharedPtr = newPtr
	//	}
	//private:
	//	sp<T> sharedPtr;
	//	sp<EventForwarder<T>> eventForwarder = nullptr
	//};

	//////////////////////////////////////////////////////////
	//// lifetime pointer alias
	//////////////////////////////////////////////////////////
	//template<typename T>
	//using lp = LifetimePointer<T>;

}

