#include "..\..\SAGameEntity.h"
#include "memory"
namespace
{
	class GameEntityTest : public SA::GameEntity
	{};

	class UserType
	{};

	void true_main()
	{
		using namespace SA;

		//never make game entity objects on stack, but this is testing whether it will compile without including sp
		//GameEntityTest obj;

		//gives warning C5046: 'SA::new_sp': Symbol involving type with internal linkage not defined without including shared pointer aliases at bottom of game entity
		std::shared_ptr<GameEntityTest> testobj = new_sp<GameEntityTest>();

		//quick test sp type checking behavior
		//sp<GameEntityTest> gameObj = new_sp<GameEntityTest>();
		//sp<UserType> nonGameObj = new_sp<UserType>();
	}
}

//int main()
//{
//	true_main();
//}