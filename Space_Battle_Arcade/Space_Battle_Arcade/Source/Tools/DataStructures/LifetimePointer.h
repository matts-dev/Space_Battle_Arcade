#pragma once

#include <vector>

#include "MultiDelegate.h"
#include "GameFramework/SAGameEntity.h"
#include "../../GameFramework/Interfaces/SATickable.h"
#include "../RemoveSpecialMemberFunctionUtils.h"


;namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tool that caches a pointer and deletes it on the next frame.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class FrameDeferredEntityDeleter : public GameEntity, public ITickable, private RemoveCopies
	{
	public:
		static FrameDeferredEntityDeleter& get();
		/** Public for new_sp syntax. Recommended you use the static getter rather than creating your own instances of this */
		FrameDeferredEntityDeleter() = default;
		void deleteLater(const sp<const GameEntity>& entity);
	private:
		virtual bool tick(float dt_sec);
	protected:
		virtual void postConstruct() override;
	private:
		std::vector<sp<const GameEntity>> pendingDelete;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Life time pointer - Shared pointer that automatically releases when an object is destroyed.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class LifetimePointer final
	{
	private:
		template<class Y>
		void entityCompileCheck() const { static_assert(std::is_base_of<SA::GameEntity, Y>::value, "LIFETIME_PTR:lifetime pointers must point to a game entity."); }

		template<class Y>
		void isChildTypeCompileCheck() const { static_assert(std::is_base_of<T, Y>::value, "LIFETIME_PTR: pointer type is not a child class of lifetime pointer's contained type."); }

	public: //api
		inline operator bool()	const noexcept { return sharedPtr.get() != nullptr; }
		inline T* operator->()	const noexcept { return sharedPtr.get(); }
		inline T& operator*()	const noexcept { return *sharedPtr; }
		inline T* get()			const noexcept { return sharedPtr.get(); }
		//inline operator std::nullptr_t()	const noexcept { return sharedPtr.get(); } //#TODO fix this when there's not other compile issues going on
		inline operator sp<T>() const noexcept { return sharedPtr; }
		inline operator wp<T>() const noexcept { return sharedPtr; }
		inline const sp<T>& toSP() const noexcept { return sharedPtr; }
		//template<class Y> inline operator sp<Y>() const noexcept { isChildTypeCompileCheck<Y>(); return sharedPtr; }
		//template<class Y> inline operator wp<Y>() const noexcept { isChildTypeCompileCheck<Y>(); return sharedPtr; }

	public: //constructors
		LifetimePointer() : sharedPtr()
		{
			entityCompileCheck<T>();
		}

		template<typename Y>
		LifetimePointer(const sp<Y>& copySP)
		{
			entityCompileCheck<Y>();
			isChildTypeCompileCheck<Y>();
			bindToPtr(copySP);
		}

		template<typename Y>
		LifetimePointer(const wp<Y>& copyWP)
		{
			entityCompileCheck<Y>();
			isChildTypeCompileCheck<Y>();
			if (!copyWP.expired())
			{
				bindToPtr(copyWP.lock());
			}
		}

		LifetimePointer(std::nullptr_t nullptrvalue)
		{
			entityCompileCheck<T>();
			bindToPtr(sp<T>(nullptr));
		}

		LifetimePointer(LifetimePointer&& move)
		{
			//#TODO implement moves where the event forwarder is shared -- this will be a perf win for containers of LP;
			//#TODO move will need to be unit tested which is why I cannot do it right now; short on time before heading in for holidays.
			//create a fresh new event forwarder so we can just references to owner, rather than pointers.
			if (*this != move)
			{
				bindToPtr(std::move(move.sharedPtr));
			}
		}
		LifetimePointer(const LifetimePointer& copy) 
		{
			if (*this != copy)
			{
				bindToPtr(copy.sharedPtr);
			}
		}
		LifetimePointer& operator=(const LifetimePointer& copy) 
		{
			if (*this != copy)
			{
				bindToPtr(copy.sharedPtr);
			}
			return *this;
		}
		LifetimePointer& operator=(LifetimePointer&& move) 
		{ 
			if (*this != move)
			{
				bindToPtr(std::move(move.sharedPtr));
			}
			return *this;
		}

		template<typename Other>
		friend class LifetimePointer;

		template<typename OtherType>
		LifetimePointer(const LifetimePointer<OtherType>& copy)
		{
			static_assert(std::is_base_of<T, OtherType>::value , "LIFETIME_PTR: attempting to copy a ptr that isn't of same type or child type.");
			bindToPtr(std::static_pointer_cast<T>(copy.sharedPtr));
		}

		template<typename OtherType>
		LifetimePointer<T>& operator=(const LifetimePointer<OtherType>& copy)
		{
			static_assert(std::is_base_of<T, OtherType>::value, "LIFETIME_PTR: attempting to copy assign a ptr that isn't of same type or child type.");
			bindToPtr(std::static_pointer_cast<T>(copy.sharedPtr));
			return *this;
		}

		template<typename OtherType>
		LifetimePointer(LifetimePointer<OtherType>&& move)
		{
			static_assert(std::is_base_of<T, OtherType>::value, "LIFETIME_PTR: attempting to move a ptr that isn't of same type or child type.");
			bindToPtr(std::move(std::static_pointer_cast<T>(move.sharedPtr)));
		}

		template<typename OtherType>
		LifetimePointer<T>& operator=(LifetimePointer<OtherType>&& move)
		{
			static_assert(std::is_base_of<T, OtherType>::value, "LIFETIME_PTR: attempting to move assign a ptr that isn't of same type or child type.");
			bindToPtr(std::move(std::static_pointer_cast<T>(move.sharedPtr)));
			return *this;
		}

		~LifetimePointer()
		{
			if (sharedPtr)
			{
				if (eventForwarder)
				{
					//eventForwarder binds strongly for speed, make sure we don't leave a dangling pointer
					eventForwarder->release(*sharedPtr);
				}

				if (sharedPtr.use_count() == 1 && !GameBase::isEngineShutdown())
				{
					FrameDeferredEntityDeleter::get().deleteLater(sharedPtr);
					sharedPtr = nullptr;
				}
			}
		}

	private:
		struct EventForwarder : public GameEntity
		{
			EventForwarder(LifetimePointer<T>& owner) : owningLP(owner) {}
			void bind(const GameEntity& entity) { 
				//the destroyed event is created as non-const within GameEntity constructor. So this const-cast should always be safe
				//I'd prefer to keep subscriptions forbidden on intentionally const delegates, so doing the cast here.
				const_cast<GameEntity&>(entity).onLifetimeOverEvent->addStrongObj(sp_this(), &EventForwarder::handleLifetimeOver); 
			}
			void release(const GameEntity& entity) {
				//see note about const cast in bind
				const_cast<GameEntity&>(entity).onLifetimeOverEvent->removeStrong(sp_this(), &EventForwarder::handleLifetimeOver);
			}
			void handleLifetimeOver() { owningLP.notifyDestroyed(); }
			LifetimePointer<T>& owningLP;
		};

		friend EventForwarder;
		void notifyDestroyed()
		{
			eventForwarder->release(*sharedPtr);
			if (sharedPtr.use_count() == 1)
			{
				FrameDeferredEntityDeleter::get().deleteLater(sharedPtr);
			}
			sharedPtr = nullptr;
		}

		template <typename Y>
		void bindToPtr(const sp<Y>& newPtr)
		{
			isChildTypeCompileCheck<Y>();
			if (!eventForwarder)
			{
				eventForwarder = new_sp<EventForwarder>( *this );
			}

			if (sharedPtr)
			{
				eventForwarder->release(*sharedPtr);
			}

			if (newPtr && !newPtr->isPendingDestroy())
			{
				sharedPtr = newPtr;
				eventForwarder->bind(*sharedPtr);
			}
			else
			{
				sharedPtr = nullptr;
			}
		}
	private:
		sp<T> sharedPtr;
		sp<EventForwarder> eventForwarder = nullptr;
	};

	template<typename A, typename B>
	inline bool operator== (const LifetimePointer<A>& ptrA, const LifetimePointer<B>& ptrB)
	{
		//open up the types of the lifetime pointers in case one is base class of the other. But static assert to be sure
		static_assert(std::is_base_of<A, B>::value || std::is_base_of<B, A>::value, "LIFETIME_PTR: attempting to compare pointers of unrelated types.");

		//let compiler do casts between pointers for us
		return ptrA.get() == ptrB.get(); 
	}
	template<typename A, typename B>
	inline bool operator!= (const LifetimePointer<A>& ptrA, const LifetimePointer<B>& ptrB)
	{
		return !(ptrA == ptrB);
	}

	//smart pointer conversions, don't allow any implicit creation of lifetime pointers
	template<typename A, typename B>
	inline bool operator== (const sp<A>& ptrA, const LifetimePointer<B>& ptrB){ return ptrA.get() == ptrB.get();}

	template<typename A, typename B>
	inline bool operator!= (const sp<A>& ptrA, const LifetimePointer<B>& ptrB){ return !(ptrA == ptrB);}

	template<typename A, typename B>
	inline bool operator== (const LifetimePointer<A>& ptrA, const sp<B>& ptrB){ return ptrA.get() == ptrB.get();}

	template<typename A, typename B>
	inline bool operator!= (const LifetimePointer<A>& ptrA, const sp<B>& ptrB) { return !(ptrA == ptrB);}

	//forbid weak pointer comparisions
	template<typename A, typename B>
	inline bool operator== (const wp<A>& ptrA, const LifetimePointer<B>& ptrB) { static_assert(false, "LIFETIME_PTR: do not directly compare weak pointers to lifetime pointers; this is expensive and should be done manually if really  required (ie locking the weak pointer into a shared ptr)."); return false; }
	template<typename A, typename B>
	inline bool operator!= (const wp<A>& ptrA, const LifetimePointer<B>& ptrB) { return !(ptrA == ptrB); }
	template<typename A, typename B>
	inline bool operator== (const LifetimePointer<A>& ptrA, const wp<B>& ptrB) { static_assert(false, "LIFETIME_PTR: do not directly compare weak pointers to lifetime pointers; this is expensive and should be done manually if really  required (ie locking the weak pointer into a shared ptr)."); return false; }
	template<typename A, typename B>
	inline bool operator!= (const LifetimePointer<A>& ptrA, const wp<B>& ptrB) { return !(ptrA == ptrB); }


	////////////////////////////////////////////////////////
	// lifetime pointer alias
	////////////////////////////////////////////////////////
	template<typename T>
	using lp = LifetimePointer<T>;
}