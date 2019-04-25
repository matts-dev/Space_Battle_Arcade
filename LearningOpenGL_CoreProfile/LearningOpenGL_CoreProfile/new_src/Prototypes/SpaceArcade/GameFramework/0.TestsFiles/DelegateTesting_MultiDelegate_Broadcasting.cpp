#include "..\..\Rendering\SAWindow.h"
#include "..\..\Tools\SmartPointerAlias.h"
#include "..\..\Rendering\OpenGLHelpers.h"
#include "..\SAGameEntity.h"
#include "..\..\Tools\DataStructures\MultiDelegate.h"
#include<memory>
#include <functional>


namespace
{
	class MouseTracker : public SA::GameEntity
	{
	public:
		void bindDelegate(MultiDelegate<double, double>& posDelegate)
		{
			//different ways to get a shaped ptr in game framework
			sp<MouseTracker> a = std::static_pointer_cast<MouseTracker>(shared_from_this());
			sp<MouseTracker> b = sp_this_impl<MouseTracker>();
			sp<MouseTracker> c = sp_this();

			std::cout << this << " : binding mouse pos callback" << std::endl;

			//binding internally allow binding of private functions
			posDelegate.addStrongObj(c, &MouseTracker::posUpdated);
		} 
	private:
		void posUpdated(double xpos, double ypos){
			std::cout << "this "<< this << " pos changed callback" << xpos << " " << ypos << std::endl;
		}
	};

	class ADifferentMouseTracker : public SA::GameEntity{
		void posUpdated(double xpos, double ypos) { std::cout << "this " << this << " pos changed callback" << xpos << " " << ypos << std::endl;}
	public:
		void bindDelegate(MultiDelegate<double, double>& posDelegate){
			//sp_this() compiles for in-line usage!
			std::cout << this << " : binding mouse pos callback" << std::endl;
			posDelegate.addStrongObj(sp_this(), &ADifferentMouseTracker::posUpdated);
		}
	};

	class WeakTracker : public SA::GameEntity{
	public:
		void posUpdated(double xpos, double ypos) { std::cout << "this " << this << " pos changed callback" << xpos << " " << ypos << std::endl; }
		void secondaryBindFunc(double xpos, double ypos) { std::cout << "SECONDARY this " << this << xpos << " " << ypos << std::endl; }
	};

	class ChildWeakTracker : public WeakTracker
	{
		void childUpdate(double xpos, double ypos) { std::cout << "this " << this << " CHILD callback" << xpos << " " << ypos << std::endl; }
	};

	class CallbackTester : public SA::GameEntity
	{
	public:
		MultiDelegate<const int, float&, const bool&> complexPrimitivesCopy;
		void handleComplexPrim(const int cint, float& floatref, const bool& crefb)
		{
			std::cout << "ci:" <<cint << " f:" << floatref << " rval bool: " << crefb << std::endl;
			floatref += 1.0f;
 		}

	};

	int trueMain()
	{
		///test simple subscription
		MultiDelegate<double, double> posDelegate;

		sp<MouseTracker> mt = new_sp<MouseTracker>();
		mt->bindDelegate(posDelegate); //bind to a private method internally

		sp<ADifferentMouseTracker> admt = new_sp<ADifferentMouseTracker>();
		admt->bindDelegate(posDelegate); //bind to a private method internally

		sp<WeakTracker> weakTracker = new_sp<WeakTracker>();
		posDelegate.addWeakObj(weakTracker, &WeakTracker::posUpdated); //bind public method 

		posDelegate.broadcast(5.0, -3.0);
		
		//below is restricted because it tries to bind a private function
		//this is probably a good thing. One can compile the binding if done within the scope of the class!
		//posDelegate.addObj(mt, &MouseTracker::posUpdated); //private functions inaccessible from outside class scope

		/// Test removal of bindings (strong and weak)
		posDelegate.removeAll(mt);
		posDelegate.removeAll(admt);
		posDelegate.removeAll(weakTracker);
		posDelegate.broadcast(1, 1);

		std::cout << "Testing multiple bindings of different functions and removal of selected functions" << std::endl;
		posDelegate.addStrongObj(weakTracker, &WeakTracker::posUpdated);
		posDelegate.addStrongObj(weakTracker, &WeakTracker::secondaryBindFunc);
		posDelegate.addWeakObj(weakTracker, &WeakTracker::posUpdated);
		posDelegate.addWeakObj(weakTracker, &WeakTracker::secondaryBindFunc);
		posDelegate.broadcast(-100, 100); //should show 4 updates

		posDelegate.removeWeak(weakTracker, &WeakTracker::posUpdated);
		posDelegate.removeStrong(weakTracker, &WeakTracker::posUpdated);
		posDelegate.broadcast(-50, 50); //should do 2 updates
		posDelegate.removeStrong(weakTracker, &WeakTracker::secondaryBindFunc);
		posDelegate.removeWeak(weakTracker, &WeakTracker::secondaryBindFunc);


		/// test inheritance subscription
		sp<ChildWeakTracker> childwt= new_sp<ChildWeakTracker>();
		//posDelegate.addStrongObj(childwt, &ChildWeakTracker::posUpdated); //doesn't compile
		//posDelegate.addStrongObj(childwt, &WeakTracker::posUpdated); //doesn't compile
		posDelegate.addStrongObj(std::static_pointer_cast<WeakTracker>(childwt), &ChildWeakTracker::posUpdated); //compiles
		posDelegate.addStrongObj(std::static_pointer_cast<WeakTracker>(childwt), &WeakTracker::posUpdated); //compiles
		posDelegate.broadcast(-2.5, -2.5);
		posDelegate.removeStrong(std::static_pointer_cast<WeakTracker>(childwt), &ChildWeakTracker::posUpdated); //compiles
		posDelegate.removeStrong(std::static_pointer_cast<WeakTracker>(childwt), &WeakTracker::posUpdated);		//compiles

		///test adding same delegate twice (more ore more) and removals sequentially
		std::cout << std::endl << std::endl << "testing over subscription" << std::endl;
		MultiDelegate<double, double> oversubDelegate;
		oversubDelegate.addStrongObj(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.addStrongObj(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(33.0, 33.0); //should see 2 broadcasts
		oversubDelegate.removeStrong(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(22.0, 22.0); //should see 1 broadcasts
		oversubDelegate.removeStrong(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(11.0, 11.0); //no subscriptions should be present

		//test weak variant
		oversubDelegate.addWeakObj(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.addWeakObj(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(33.0, 33.0); //should see 2 broadcasts
		oversubDelegate.removeWeak(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(22.0, 22.0); //should see 1 broadcasts
		oversubDelegate.removeWeak(weakTracker, &WeakTracker::posUpdated);
		oversubDelegate.broadcast(11.0, 11.0); //no subscriptions should be present

		/// Test copying delegates (NOT CURRENTLY SUPPORTED)
		std::cout << std::endl << std::endl;

		MultiDelegate<const int, float&, const bool&> complexPrimitives;
		sp<CallbackTester> cbTester = new_sp<CallbackTester>();
		complexPrimitives.addWeakObj(cbTester, &CallbackTester::handleComplexPrim);
		complexPrimitives.addStrongObj(cbTester, &CallbackTester::handleComplexPrim);
		//cbTester->complexPrimitivesCopy = complexPrimitives;

		//int xi = 5;
		//const int cxi = 10;
		//float yfloat = 3.0f;
		//complexPrimitives.broadcast(xi, yfloat, /*intention rvalue*/ false);
		//complexPrimitives.removeAll(cbTester);

		////test copy broadcast after removal
		//cbTester->complexPrimitivesCopy.broadcast(cxi, yfloat, true);
		/// Test copying during broadcast (probably requires an internal delegate for update)? (NOT CURRENTLY SUPPORTED)

		/// Test removal during broadcast

		/// Test add during broadcast

		/// test Lvalue ref bindings
		/// test RValue ref bindings
		/// test const values
		/// const delegates
		/// test delegate causing removal of delegate binding while broadcasting
		/// Test strong bindings
		/// Test Weak Bindings Scope Clearing
		/// Test removing stale delegates 


		//testing usage of std::bind; I don't think this will let me get entire delegate behavior within a single stack frame
		//ADifferentMouseTracker obj2;
		//modify posUpdate to be public to get below to compile
		//auto bindfunc = std::bind(&ADifferentMouseTracker::posUpdated, &obj2, std::placeholders::_1, std::placeholders::_2);
		//bindfunc(5.0, 5.0);


		return 0;
	}
}

int main()
{
	return trueMain();
}