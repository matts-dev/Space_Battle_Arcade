#pragma once

#include "..\..\GameFramework\SAGameEntity.h"
#include <stdexcept>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <utility>
#include <iterator>
#include <assert.h>
#include <vector>
#include <functional>

namespace SA
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dev Notes:
	// I considered making subscribers a super class to both weak and strong subscribers, but that will require dynamic casting within
	// the delegate itself to determine it is should remove stale weak pointers. It also begs to have a shared virtual function
	// "invoke" that checks if the pointer is valid in the weak pointer case and immediately calls the function in the shared pointer case.
	// I believe the overhead of virtual functions can be avoided here by just maintaining different lists of subscribers. Thus, the
	// need for true polymorphic subscriber parent class is very small. 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dev Concerns / Todos
	//
	//---Variadic argument concern---
	//	declarations like:
	//			struct StrongAgnosticSubscriber : public Subscriber<Args...>
	//	 May should be 
	//			struct StrongAgnosticSubscriber : public Subscriber<Args&&...>
	//
	//	Both of the above compile.
	//
	//   I believe the <Args...> variant should be okay.
	//	 Since Args is a collection of types, eg {int, double&, char&&}, I do not think the lvalue/rvalueness of those types is lost in the declaration of the super-class types.
	//	 Where as in functions parameters, the rvalues are given a name and become lvalues (hence requiring forwarding). eg "void foo(int&& rvalues_lvaluename)"
	//	 It doesn't seem this is the cast for passing template parameter packs? It doesn't seem like we're giving them a name and converting them to lvalues.

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	* Top level class that provides virtual invoke.
	* This is agnostic of the bound type;
	* the delegate container need not specify concrete types
	*/
	template<typename... Args>
	struct Subscriber
	{
		bool bMarkedForDelete = false;

		//void changing the return type from void, that way this can be used as the return type for return val delegates
		virtual void invoke(Args&&... args) = 0;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Strong Subscribers
	//
	// Strong subscribers maintain a shared pointer to their subscriber
	// This can cause resource leaks if delegates are not cleaned up before an object is released.
	// Becareful, Strong subscribers cannot be cleaned up in the dtor class types because the strong object itself
	// will prevent the dtor from ever being called.
	//
	// Strong subscribers may provide a performance benefit compared to weak subscribers. Weak subscribers require locking
	// of a weak pointer, which will require atomic thread operations, which are generally slow.
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/** Holds a shared_ptr to an object, delegate will keep object alive if it is not removed*/
	template<typename... Args>
	struct StrongAgnosticSubscriber : public Subscriber<Args...>
	{
		//Type is always GameEntity; this allows containers to hold multiple types
		//and use polymorphism mechanisms to invoke on different types
		sp<SA::GameEntity> strongObj;

		StrongAgnosticSubscriber(const sp<SA::GameEntity>& obj) : strongObj(obj) {}
	};

	/** Strong subscriber that knows it concrete type and therefore can invoke the delegate callback */
	template<typename T, typename... Args>
	struct StrongSubscriber : public StrongAgnosticSubscriber<Args...>
	{
		static_assert(std::is_base_of<SA::GameEntity, T>::value, "Smart Subscribers must be GameEntity Objects.");

		StrongSubscriber(const sp<T>& obj, void(T::*fptr)(Args...))
			: StrongAgnosticSubscriber<Args...>(obj), boundFunc(fptr)
		{
		}

		using MemberFunc = void(T::*)(Args...);
		MemberFunc boundFunc;

		virtual void invoke(Args&&... args) override
		{
			//MODIFICATION NOTE: preserve that repeatedly "step into" when debugging quickly gets to callbacks and not other functions

			//static cast is safe because game entity is a required base class
			//collapsing above code into one line for faster debugging
			((*std::static_pointer_cast<T>(this->strongObj)).*boundFunc)(std::forward<Args>(args)...);
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Weak Subscribers
	//
	// Weak subscribers hold weak pointers to their subscribers
	// This are less bug prone because they will be automatically cleaned up if the subscriber object has went out of scope
	// and been destroyed. 
	//
	// However, there is some overhead associated with using weak pointers. Weak pointers require an thread atomic operation
	// to access the control block. This is probably the equivalent of locking a mutex when ever a shared pointer is obtained
	// from the weak pointer.
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	/** Holds a weak pointer to the object, delegate will not exert control over pointer liftime */
	template<typename... Args>
	struct WeakAgnosticSubscriber : public Subscriber<Args...>
	{
		//Type is always GameEntity; this allows containers to hold multiple types
		//and use polymorphism mechanisms to invoke on different types
		wp<SA::GameEntity> weakObj;

		/** Intentionally not using delegate system to avoid recusrive complexities*/
		std::function<void()> notifyStaleSubscriber;

		WeakAgnosticSubscriber(const sp<SA::GameEntity>& obj) : weakObj(obj) {}
	};

	/** Weak subscriber that knows it concrete type and therefore can invoke the delegate callback */
	template<typename T, typename... Args>
	struct WeakSubscriber : public WeakAgnosticSubscriber<Args...>
	{
		static_assert(std::is_base_of<SA::GameEntity, T>::value, "Smart Subscribers must be GameEntity Objects.");

		WeakSubscriber(const sp<T> obj, void(T::*fptr)(Args...)) : WeakAgnosticSubscriber<Args...>(obj), boundFunc(fptr) {}

		using MemberFunc = void(T::*)(Args...);
		MemberFunc boundFunc;

		virtual void invoke(Args&&... args) override
		{
			//MODIFICATION NOTE: preserve that repeatedly "step into" when debugging quickly gets to callbacks and not other functions

			//lock and cast object to appropriate type for member function invoking.
			if (sp<T> obj = std::static_pointer_cast<T>(this->weakObj.lock()))
			{
				((*obj).*boundFunc)(std::forward<Args>(args)...);
			}
			else
			{
				//avoiding using return type of invoke to indicate failure because this will likely
				//be the exact same pattern for return type delegates
				this->notifyStaleSubscriber();
			}
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Delegates
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
		MultiDelegate allows multiple objects to subscribe to the delegate's event and be notified.
		Multidelegate is built for single threaded usage.
	*/
	template<typename... Args>
	class MultiDelegate /*intentionally not a game entity; perhaps it should be but then all stack allocated versions will need to be updated*/
	{
	public:
		void broadcast(Args... args)
		{
			//NOTE WHEN CHANGING: preserve that repeatedly "step into" when debugging quickly gets to callbacks and not other functions
			//Debugging delegates in other systems is extremely annoying, doing the above makes it a lot less annoying :)

			broadcasting = true;
			for (const auto& key_value_pair : strongSubscribers)	//std::map has O(n) walk
			{
				sp<StrongAgnosticSubscriber<Args...>> subscriber = key_value_pair.second;
				subscriber->invoke(std::forward<Args>(args)...);
			}
			for (const auto& key_value_pair : weakSubscribers)		//std::map has O(n) walk
			{
				sp<WeakAgnosticSubscriber<Args...>> subscriber = key_value_pair.second;
				subscriber->invoke(std::forward<Args>(args)...);
			}
			broadcasting = false;

			completeQueuedOperations();
		}

		/**
			Binds object to delegate and automatically cleans up object from delegate subscribers when object goes out of scope
			***This function should generally be the default way of binding to a delegate.***
			Relies on weak pointers internally. See WeakSubscriber comments.
		*/
		template<typename T>
		void addWeakObj(const sp<T>& obj, void(T::*fptr)(Args...))
		{
			static_assert(std::is_base_of<SA::GameEntity, T>::value, "Delegates are only supported for game entity inheriting classes.");

			//store the concrete type (T) into a type (WeakSubscriber) that hides (WeakAgnosticSubscriber) the concrete template type for storage
			sp<WeakAgnosticSubscriber<Args...>> subscriber = new_sp<WeakSubscriber<T, Args...>>(obj, fptr);

			//a new lambda will need to be set if delegate copying behavior is ever supported; or an array of lambdas if shared subscribers
			subscriber->notifyStaleSubscriber = [this]() { this->bDetecftedStaleWeakSubscriber = true; };

			//add must consider if broadcasting is happening
			if (!broadcasting)
			{
				SA::GameEntity* objAddress = obj.get();
				weakSubscribers.emplace(objAddress, subscriber);
			}
			else
			{
				pendingWeakAdds.push_back(subscriber);
			}
		}


		/**
			Will strongly bind to object, preventing it from being destroyed. This bug prone and should be avoided unless explicity
			seeking the performance gains of strong subscribers (see comments). These bindings cannot undone within dtor of subscribing
			object, preventing typically RAII behavior. If you desire RAII behavior, add object bindings using weak subscribers.
		*/
		template<typename T>
		void addStrongObj(const sp<T>& obj, void(T::*fptr)(Args...))
		{
			static_assert(std::is_base_of<SA::GameEntity, T>::value, "Delegates are only supported for game entity inheriting classes.");

			//store the concrete type (T) into a type (StrongSubscriber) that hides (StrongAgnosticSubscriber) the concrete template type for storage
			sp<StrongAgnosticSubscriber<Args...>> subscriber = new_sp<StrongSubscriber<T, Args...>>(obj, fptr);

			//add must consider if broadcasting is happening
			if (!broadcasting)
			{
				SA::GameEntity* objAddress = obj.get();
				strongSubscribers.emplace(objAddress, subscriber);
			}
			else
			{
				pendingStrongAdds.push_back(subscriber);
			}
		}


		template<typename T>
		void removeAll(const sp<T>& obj)
		{
			static_assert(std::is_base_of<SA::GameEntity, T>::value, "Delegates are only supported for game entity inheriting classes.");

			//remove must consider if broadcasting is happening
			if (!broadcasting)
			{
				strongSubscribers.erase(obj.get());
				weakSubscribers.erase(obj.get());
			}
			else
			{
				//It's a design decision to not tag "pending delete" delegates while broadcasting
				//because that will lead to race conditions on order of delegate subscription.
				//It seems better that all delegates finish their broadcast rather than have potentially 
				//other delegates deregister a pending delegate (within other's invoke) before pending's invoke is hit.
				QueuedRemoveAlls.emplace_back(std::static_pointer_cast<SA::GameEntity>(obj));
			}
		}

		template<typename T>
		void removeStrong(const sp<T>& obj, void(T::*fptr)(Args...))
		{
			static_assert(std::is_base_of<SA::GameEntity, T>::value, "Delegates are only supported for game entity inheriting classes.");

			const std::pair<StrongIter, StrongIter>& start_end = strongSubscribers.equal_range(obj.get());
			StrongIter start = start_end.first;
			StrongIter end = start_end.second;

			for (StrongIter iter = start; iter != end; ++iter)
			{
				const sp<StrongAgnosticSubscriber<Args...>>& sub = iter->second;
				SA::GameEntity* key = iter->first;

				assert(key == obj.get());

				/**A dynamic_point_cast might be necessary, but trying to avoid. The member function belong to type T
				theoretically we can just static cast if object matches and compare the function addresses.
				Inheritance might complicate this, that is if someone binds a super class function with a child class obj;
				Ts will not match in that case so function doesn't compile. But There may be cases where same obj might be
				registered twice (once as super, once as child) and thus it seems static_casting is inappropriate. To avoid
				this entirely, I could return handles and remove via handles but I'd like to avoid that.
				*/
				if (sp<StrongSubscriber<T, Args...>> castSub = std::dynamic_pointer_cast<StrongSubscriber<T, Args...>>(sub))
				{
					if (castSub->boundFunc == fptr && !castSub->bMarkedForDelete)
					{
						if (!broadcasting)
						{
							strongSubscribers.erase(iter);
						}
						else
						{
							//according to cppreference.com multimap iterators are not invalidated from insertions/erases of iterators
							//so we can cache the iterators to remove until broadcasting is done
							castSub->bMarkedForDelete = true;
							pendingStrongRemoves.push_back(iter);
						}

						return; //early out, we've found the removal; duplicate adds should be handled with duplicate removes
					}
				}
			}
		}

		template<typename T>
		void removeWeak(const sp<T>& obj, void(T::*fptr)(Args...))
		{
			static_assert(std::is_base_of<SA::GameEntity, T>::value, "Delegates are only supported for game entity inheriting classes.");

			const std::pair<WeakIter, WeakIter>& start_end = weakSubscribers.equal_range(obj.get());
			WeakIter start = start_end.first;
			WeakIter end = start_end.second;

			for (WeakIter iter = start; iter != end; ++iter)
			{
				const sp<WeakAgnosticSubscriber<Args...>>& sub = iter->second;

				SA::GameEntity* key = iter->first;

				assert(key == obj.get());

				/** see addStrong for notes on dynamic cast vs static cast potential */
				if (sp<WeakSubscriber<T, Args...>> castSub = std::dynamic_pointer_cast<WeakSubscriber<T, Args...>>(sub))
				{
					//do not check for expired weak ptrs if this is used as mechanism to remove stale bindings
					if (castSub->boundFunc == fptr && !castSub->bMarkedForDelete)
					{
						if (!broadcasting)
						{
							weakSubscribers.erase(iter);
						}
						else
						{
							castSub->bMarkedForDelete = true;
							pendingWeakRemoves.push_back(iter);
						}

						return; //early out, we've found the removal; duplicate adds should be handled with duplicate removes
					}
				}
			}
		}

		void completeQueuedOperations()
		{
			//must not be broadcasting when this is called, otherwise we can get in an infinite loop on adds
			assert(!broadcasting);

			for (const sp<SA::GameEntity>& entity : QueuedRemoveAlls)
			{
				//remove all this
				removeAll(entity);
			}

			for (StrongIter iter : pendingStrongRemoves)
			{
				strongSubscribers.erase(iter);
			}
			pendingStrongRemoves.clear();

			//must come before pending weak removes
			if (bDetecftedStaleWeakSubscriber)
			{
				//O(n) walk over std::map
				std::vector<WeakIter> toRemove;
				for (WeakIter iter = weakSubscribers.begin(); iter != weakSubscribers.end(); ++iter)
				{
					sp<WeakAgnosticSubscriber<Args...>> subscriber = iter->second;
					if (subscriber->weakObj.expired())
					{
						pendingWeakRemoves.push_back(iter);
					}
				}
			}

			for (WeakIter iter : pendingWeakRemoves)
			{
				weakSubscribers.erase(iter);
			}
			pendingWeakRemoves.clear();


			for (sp<StrongAgnosticSubscriber<Args...>>& newStrongSub : pendingStrongAdds)
			{
				SA::GameEntity* objAddress = newStrongSub->strongObj.get();
				strongSubscribers.emplace(objAddress, newStrongSub);
			}
			pendingStrongAdds.clear();

			for (sp<WeakAgnosticSubscriber<Args...>> newWeakSub : pendingWeakAdds)
			{
				if (sp<SA::GameEntity> obj = newWeakSub->weakObj.lock())
				{
					weakSubscribers.emplace(obj.get(), newWeakSub);
				}
			}
			pendingWeakAdds.clear();
		}

		std::size_t numBound() { return numStrong() + numWeak(); }
		std::size_t numStrong() { return strongSubscribers.size(); }
		std::size_t numWeak() { return weakSubscribers.size(); }

	public:
		MultiDelegate() = default;

		////////////////////////////////////////////
		// Reasons not to support copyable delegates:
		// *it allows for easy accidental errors where passing delegates not by reference, but by value, can go unnoticed
		// *it makes testing delegates simpler for changing
		// *smart pointer delegates 
		// *a factory function for copying can be created to avoid accidental copies
		///////////////////////////////////////////
#ifdef MULTIDELEGATE_SUPPORT_COPY_CTORS
		MultiDelegate(const MultiDelegate<Args...>& copy)
		{
			//when copying, intentionally do not copy state of broadcasting or pending adds/removes
			//this might not be a great design decision, and it can be changed. But edge cases need
			//to have special consideration. I am considering deleting copy entirely for simpler API
			this->strongSubscribers = copy.strongSubscribers;
			this->weakSubscribers = copy.weakSubscribers;
			throw std::runtime_error("copying stale weak delegate binding not yet implemented");
#ifndef SUPPRESS_BROADCAST_COPY_WARNINGS
			if (copy.broadcasting)
			{
				std::cerr << "Copied Delegate while broadcasting, this may cause hard to detect bugs! See special member functions of multidelegate to supress this warning" << std::endl;
			}
#endif //SUPPRESS_BROADCAST_COPY_WARNINGS
		}
		MultiDelegate<Args...>& operator=(const MultiDelegate<Args...>& copy)
		{
			//copying while broadcasting does not account for queued actions (adds/removes/etc). See copy ctor 
			this->strongSubscribers = copy.strongSubscribers;
			this->weakSubscribers = copy.weakSubscribers;
			throw std::runtime_error("copying stale weak delegate binding not yet implemented");
#ifndef SUPPRESS_BROADCAST_COPY_WARNINGS
			if (copy.broadcasting)
			{
				std::cerr << "Copied Delegate while broadcasting, this may cause hard to detect bugs! See special member functions of multidelegate to supress this warning" << std::endl;
			}
#endif //SUPPRESS_BROADCAST_COPY_WARNINGS
			return *this;
		}
#else
	/** Use smart pointer managed delegates to achieve pass-able delegates; or define the macro for copy ctors */
		MultiDelegate(const MultiDelegate<Args...>& copy) = delete;
		MultiDelegate<Args...>& operator=(const MultiDelegate<Args...>& copy) = delete;
#endif //MULTIDELEGATE_SUPPORT_COPY_CTORS

		//Move semantics are not supported unless testing is available for them
		MultiDelegate(MultiDelegate<Args...>&& move) = delete;
		MultiDelegate<Args...>& operator=(MultiDelegate<Args...>&& move) = delete;


	private: //these are explicitly ordered for appearance in debugger!

		/* type independent storage for subscribers; uses virtual function to invoke member functions */
		std::multimap< SA::GameEntity*, sp<StrongAgnosticSubscriber<Args...>> > strongSubscribers;
		std::multimap< SA::GameEntity*, sp<WeakAgnosticSubscriber<Args...>> > weakSubscribers;

		/* flag for reentrant code that prevents add/remove operations from occurring during broadcasting */
		bool broadcasting = false;

		/* Storage of removals queued up during a broadcast*/
		using StrongIter = typename decltype(strongSubscribers)::iterator;
		using WeakIter = typename decltype(weakSubscribers)::iterator;
		std::vector<StrongIter> pendingStrongRemoves;
		std::vector<WeakIter> pendingWeakRemoves;

		/* Storage for adds queued up during a broadcast  */
		std::vector< sp<StrongAgnosticSubscriber<Args...>> > pendingStrongAdds;
		std::vector< sp<WeakAgnosticSubscriber<Args...>> > pendingWeakAdds;

		std::vector<sp<SA::GameEntity>> QueuedRemoveAlls;

		bool bDetecftedStaleWeakSubscriber = false;
	};
}