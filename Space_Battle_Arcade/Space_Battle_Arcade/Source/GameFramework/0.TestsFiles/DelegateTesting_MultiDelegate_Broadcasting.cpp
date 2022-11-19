#include "Rendering/SAWindow.h"
#include "Rendering/OpenGLHelpers.h"
#include "GameFramework/SAGameEntity.h"
#include "Tools/DataStructures/MultiDelegate.h"
#include<memory>
#include <functional>
#include <string>


namespace SA
{
	class MouseTracker : public SA::GameEntity
	{
	public:
		void bindDelegate(SA::MultiDelegate<double, double>& posDelegate)
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
		void bindDelegate(SA::MultiDelegate<double, double>& posDelegate){
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

	class UserTypeToPass
	{
	public:
		std::string value = "default";

		void RLValueType() & { std::cout << "LVALUE" << std::endl; }
		void RLValueType() && { std::cout << "rvalue" << std::endl; }
	};

	class CallbackTester : public SA::GameEntity
	{
	public:
		SA::MultiDelegate<const int, float&, const bool&> complexPrimitivesCopy;
		void handleComplexPrim(const int cint, float& floatref, const bool& crefb)
		{
			std::cout << "ci:" << cint << " f:" << floatref << " rval bool: " << crefb << std::endl;
			floatref += 1.0f;
		}

		void handleUserType(const UserTypeToPass cCopy, UserTypeToPass& ref, const UserTypeToPass& cRef)
		{
			std::cout << cCopy.value << std::endl;
			std::cout << ref.value << std::endl;
			std::cout << cRef.value << std::endl;

			//change ref between broadcasts
			ref.value += std::string("+");
		}

		void handleRLValueTest(UserTypeToPass& lvalue, UserTypeToPass&& rvalue)
		{
			lvalue.RLValueType();
			std::move(rvalue).RLValueType(); //this will always report rvalue; real test is to pass rvalues to this function
		}

		//self removals
		sp<MultiDelegate<>> sharedNoArgDelegate = nullptr;
		void HandleRemoveWeakDuringBroadcast()
		{
			std::cout << "removing WEAK self within callback!" << std::endl;
			sharedNoArgDelegate->removeWeak(sp_this(), &CallbackTester::HandleRemoveWeakDuringBroadcast);
		}
		void HandleRemoveStrongDuringBroadcast()
		{
			std::cout << "removing WEAK self within callback!" << std::endl;
			sharedNoArgDelegate->removeStrong(sp_this(), &CallbackTester::HandleRemoveStrongDuringBroadcast);
		}

		//test additions during broadcast
		sp<MultiDelegate<>> addDelegate = nullptr;
		void HandleAddDuringBroadcast()
		{
			addDelegate->addStrongObj(sp_this(), &CallbackTester::AddedCallback);
			addDelegate->addWeakObj(sp_this(), &CallbackTester::AddedCallback);
		}

		void AddedCallback()
		{
			//call back used in add while broadcast testing
			std::cout << "successfully added" << std::endl;
		}


		MultiDelegate<double, double> passDelegate;
		void HandlePassedDelegateCallback(double x, double y)
		{
			std::cout << "registered to a passed delegate callback: " << x << " " << y << std::endl;;
		}

		void HandleDelegatePassed(MultiDelegate<double, double>& passedDelegate)
		{
			std::cout << "passed delegate!" << std::endl;;
			passedDelegate.addWeakObj(sp_this(), &CallbackTester::HandlePassedDelegateCallback);
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
		/// Test copying during broadcast (probably requires an internal delegate for update)? (NOT CURRENTLY SUPPORTED)


		/// test Lvalue/const ref bindings
		std::cout << std::endl << std::endl;
		MultiDelegate<const int, float&, const bool&> complexPrimitives;
		sp<CallbackTester> cbTester = new_sp<CallbackTester>();
		complexPrimitives.addWeakObj(cbTester, &CallbackTester::handleComplexPrim);
		complexPrimitives.addStrongObj(cbTester, &CallbackTester::handleComplexPrim);
		int xi = 5;
		float yfloat = 3.0f;
		complexPrimitives.broadcast(xi, yfloat, /*intention rvalue*/ false);
		complexPrimitives.removeAll(cbTester);

		///Test passing user types
		MultiDelegate<const UserTypeToPass, UserTypeToPass&, const UserTypeToPass&> UserTypeDelegate;
		UserTypeDelegate.addWeakObj(cbTester, &CallbackTester::handleUserType);

		UserTypeToPass obj1;
		UserTypeToPass obj2;
		obj2.value = "obj2";
		UserTypeToPass obj3;
		obj3.value = "3 3 3";
		UserTypeDelegate.broadcast(obj1, obj2, obj3);
		UserTypeDelegate.broadcast(obj1, obj2, obj3);
		UserTypeDelegate.removeAll(cbTester);

		///Test RValue
		MultiDelegate<UserTypeToPass&, UserTypeToPass&&> lvalue_rvalue_delegate;
		lvalue_rvalue_delegate.addWeakObj(cbTester, &CallbackTester::handleRLValueTest);
		lvalue_rvalue_delegate.broadcast(obj1, UserTypeToPass{});

		///Test no argument delegate
		MultiDelegate<> NoArgs;
		NoArgs.broadcast();

		/// Test removal during broadcast
		sp<MultiDelegate<>> sharedNoArgDelegate = new_sp<MultiDelegate<>>();
		sharedNoArgDelegate->broadcast();

		cbTester->sharedNoArgDelegate = sharedNoArgDelegate;

		//test weak variant
		sharedNoArgDelegate->addWeakObj(cbTester, &CallbackTester::HandleRemoveWeakDuringBroadcast);
		sharedNoArgDelegate->broadcast(); //this should hit delegate, and it should remove itself
		sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 

		sharedNoArgDelegate->addStrongObj(cbTester, &CallbackTester::HandleRemoveWeakDuringBroadcast);
		sharedNoArgDelegate->broadcast(); //this should hit delegate, and it should remove itself
		sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 


		/// Test add during broadcast
		sp<MultiDelegate<>> addDelegate = new_sp<MultiDelegate<>>();
		addDelegate->broadcast();
		cbTester->addDelegate = addDelegate;
		addDelegate->addWeakObj(cbTester, &CallbackTester::HandleAddDuringBroadcast);
		addDelegate->broadcast(); //by design, should not report the added delegates

		//remove the delegate that actually adds additional now that we've added them
		addDelegate->removeWeak(cbTester, &CallbackTester::HandleAddDuringBroadcast);
		addDelegate->broadcast(); //this should now call 2 callbacks, one for strong add and weak add


		/// Test broadcasting a/the delegate 
		MultiDelegate<double, double> passDelegate;
		MultiDelegate<MultiDelegate<double, double>&> delegatePasser;
		delegatePasser.addWeakObj(cbTester, &CallbackTester::HandleDelegatePassed);
		delegatePasser.broadcast(passDelegate);
		passDelegate.broadcast(123.0, 321.0);


		/// Test Weak Bindings Scope Clearing
		/// Test removing stale delegates 
		std::cout << std::endl << std::endl << std::endl;
		MultiDelegate<double, double> posUpdater;
		{
			sp<WeakTracker> scopedMT = new_sp<WeakTracker>();
			posUpdater.addWeakObj(scopedMT, &WeakTracker::posUpdated);
			posUpdater.broadcast(777.0, 777.0); // should broadcast once
		}
		posUpdater.broadcast(888.0, 888.0);


		//testing usage of std::bind; I don't think this will let me get entire delegate behavior within a single stack frame
		//ADifferentMouseTracker obj2;
		//modify posUpdate to be public to get below to compile
		//auto bindfunc = std::bind(&ADifferentMouseTracker::posUpdated, &obj2, std::placeholders::_1, std::placeholders::_2);
		//bindfunc(5.0, 5.0);


		return 0;
	}
}

//int main()
//{
//	return trueMain();
//}