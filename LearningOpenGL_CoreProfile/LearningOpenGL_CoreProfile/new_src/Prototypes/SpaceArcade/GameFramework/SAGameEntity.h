#pragma once
#include <memory>
#include "../Game/OptionalCompilationMacros.h"

namespace SA
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//These forward declarations are carefully placed here to avoid circle includes.
//new_sp is desired to be responsible for calling post construct, and therefore 
//needs to know of the type "GameEntity". So that file will include "GameEntity"
//However, game entity needs to know the smart pointer aliases to friend them, 
//hence the forward declarations
//
//Note, Some of below are the real type aliases (eg see sp)

	//shared pointer
	template<typename T>
	using sp = std::shared_ptr<T>;

	template<typename T, typename... Args>
	sp<T> new_sp(Args&&... args);

	//weak pointer
	template<typename T>
	using wp = std::weak_ptr<T>;

	//unique pointer
	template<typename T>
	using up = std::unique_ptr<T>;

	template<typename T, typename... Args>
	up<T> new_up(Args&&... args);
///////////////////////////////////////////////////////////////////////////////////////////////////
}





/** Automatically provide template type for convenience. Namespaces and class (SA::GameEntity) must be specified for certain templates to compile.*/
#define sp_this() SA::GameEntity::sp_this_impl<std::remove_reference<decltype(*this)>::type>()

/** Represents a top level object*/
namespace SA
{
	//forward declarations
	template<typename... Args>
	class MultiDelegate;

	/* Root level object that most classe should derive from to take advantage of the engine convenience structures
		eg: new_sp post construction callbacks, multidelegate subscription, etc.
	*/
	class GameEntity : public std::enable_shared_from_this<GameEntity>
 	{
	public:
		/** Game entities will all have virtual destructors to avoid easy-to-miss mistakes*/
		GameEntity();
		virtual ~GameEntity(){}

	public:
		const sp< MultiDelegate<const sp<GameEntity>&> > onDestroyedEvent;
		bool isPendingDestroy() const { return bPendingDestroy; }

		/** WARNING: think twice before using this; if you're given a ref/rawptr then the API may be trying to prevent you from holding a reference
		 * subclasses can deny this request by overriding the virtual method to return nullptr.*/
		virtual wp<GameEntity> requestReference() { return sp_this(); }

	protected:
		/* new_sp will call this function after the object has been created, allowing GameEntities 
		   to subscribe to delegates immediately after construction*/
		virtual void postConstruct() {};

		/* Marks an entity for pending destroy */
		void destroy();
		virtual void onDestroyed();

		/** Not intended to be called directly; please use macro "sp_this()" to avoid specifying template types*/
		template<typename T>
		sp<T> sp_this_impl()
		{
			//static cast safe because this must be called from derived classes 
			//static cast for speed; does not inccur RTTI overhead of dynamic cast
			return std::static_pointer_cast<T>(shared_from_this());
		}
	private:
		bool bPendingDestroy = false;

	private:
		template<typename T, typename... Args>
		friend sp<T> new_sp(Args&&... args);

	public:
		struct CleanKey { friend class GameBase;  private: CleanKey() {} };
		static void cleanupPendingDestroy(CleanKey);
	};
}

/* Smart Pointer Alias
	Bodies of smart pointers alias functions
	Must come after definition of game entity
*/
namespace SA
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Usage example
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	int main(){
		using namespace SA;

		//shared pointer
		sp<EG_Class> ptr2 = new_sp<EG_Class>();
		ptr2->Foo();

		//weak pointer
		wp<EG_Class> wp1 = ptr2;
		if (sp<EG_Class> aquired = wp1.lock())
		{
			aquired->Foo();
		}

		//unique pointer
		up<EG_Class> up1 = new_up<EG_Class>();
		up1->Foo();
	}
	*/

	template<typename T, typename... Args>
	sp<T> new_sp(Args&&... args)
	{
		if constexpr (std::is_base_of<SA::GameEntity, T>::value)
		{
			sp<T> newObj = std::make_shared<T>(std::forward<Args>(args)...);
			//safe cast because of type-trait
			GameEntity* newGameEntity = static_cast<GameEntity*>(newObj.get());
			newGameEntity->postConstruct();
			return newObj;
		}
		else
		{
			return std::make_shared<T>(std::forward<Args>(args)...);
		}
	}

	template<typename T, typename... Args>
	up<T> new_up(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

}