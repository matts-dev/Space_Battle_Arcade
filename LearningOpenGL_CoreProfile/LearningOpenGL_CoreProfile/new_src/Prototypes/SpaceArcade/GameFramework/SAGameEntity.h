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


	protected:

		/** Not intended to be called directl; please use macro "sp_this" to avoid specifying template types*/
		template<typename T>
		sp<T> sp_this_impl()
		{
			//static cast safe because this must be called from base classes 
			//static cast for speed; does not inccur RTTI overhead of dynamic cast
			return std::static_pointer_cast<T>(shared_from_this());
		}
	};
}