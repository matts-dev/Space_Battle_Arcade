#pragma once
#include <memory>
#include "..\Tools\SmartPointerAlias.h"

/** Automatically provide template type for convenience */
#define sp_this() sp_this_impl<std::remove_reference<decltype(*this)>::type>()

/** Represents a top level object*/
namespace SA
{
	class GameEntity : public std::enable_shared_from_this<GameEntity>
	{
	public:
		/** Game entities will all have virtual destructors to avoid easy-to-miss mistakes*/
		virtual ~GameEntity(){}

	protected:

		/** Not intended to be called directl; please use macro "sp_this" to avoid specifying template types*/
		template<typename T>
		sp<T> sp_this_impl()
		{
			//static cast safe because this must be called from base classes 
			//static cast for speed; does not inccur RTTI overhead of dynamic cast
			return std::static_pointer_cast<T>(shared_from_this());
		}
	private:
		//template<typename T, typename... Args>
		//friend sp<T> new_sp(Args&&... args);
		//this will need to be called on a next-tick timer to give all game-entity the opportunity to use sp_this to register to delegates
		//^^^ might not be safe because objects could be destroyed in same frame as they're created^^^
		//or there needs to be a factory method that calls this after construction is complete
		//or the new_sp function can be specialized for GameEntity types and call this function
		//virtual void postConstruct() {};
	};
}