#include "EngineTestSuite.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

namespace SA
{
	namespace DelegateTests
	{
		class MultiDelegate_UnitTest : public SA::UnitTest
		{
		public:
			MultiDelegate_UnitTest()
			{
				testNamespace = "MultiDelegate:";
			}
		};

		///BOILER PLATE
		/*
		class Test_ : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int value) { myValue = value; }
				int myValue = 0;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "_";

				int testVal = 8;

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<int> basicDelegate;
				return false;
			}
		};
		*/


		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///broadcast notifies
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_BasicBroadcast : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int value) { myValue = value; }
				int myValue = 0;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Basic Broadcast Tests (Weak/Strong)";
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<int> basicDelegate;
				basicDelegate.addWeakObj(weakUser, &User::handler);

				int testVal = 5;
				basicDelegate.broadcast(testVal);

				if (weakUser->myValue != testVal)
				{
					errorMessage = "failed weak user basic broadcast";
					return false;
				}

				sp<User> strongUser = new_sp<User>();
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.broadcast(testVal);

				if (weakUser->myValue != testVal)
				{
					errorMessage = "failed strong user basic broadcast";
					return false;
				}
				return true;
			}
		};
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// private subscription tests
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_PrivateSubscription : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				int myValue = 0;
				void bindDelegate(MultiDelegate<int>& inDelegate, bool bStrong)
				{
					if (bStrong)
					{
						inDelegate.addStrongObj(sp_this(), &User::handler);
					}
					else
					{
						inDelegate.addWeakObj(sp_this(), &User::handler);
					}
				}

				void removeDelegate(MultiDelegate<int>& inDelegate, bool bStrong)
				{
					if (bStrong)
					{
						inDelegate.removeStrong(sp_this(), &User::handler);
					}
					else
					{
						inDelegate.removeWeak(sp_this(), &User::handler);
					}
				}
			private:
				void handler(int value) { myValue = value; }
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Private Subscription Test";
				int testVal = 8;

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<int> basicDelegate;

				//-- STRONG ---------------------------------------
				strongUser->bindDelegate(basicDelegate, true);

				basicDelegate.broadcast(testVal);
				if (strongUser->myValue != testVal)
				{
					errorMessage = "strong user test values do not match after broadcast";
					return false;
				}

				strongUser->removeDelegate(basicDelegate, true);

				//-- WEAK ------------------------------------------
				weakUser->bindDelegate(basicDelegate, false);
				basicDelegate.broadcast(testVal);
				weakUser->removeDelegate(basicDelegate, false);

				if (weakUser->myValue != testVal)
				{
					errorMessage = "weak user value doesn't matcha fter braodcast";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test removal of bindings (strong and weak)
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_RemoveBindings : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int value) { myValue = value; }
				int myValue = 0;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Remove Bindings";
				int testVal = 9;

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();
				MultiDelegate<int> basicDelegate;

				//test that values update after delegate
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.addWeakObj(weakUser, &User::handler);
				basicDelegate.broadcast(testVal);
				if (strongUser->myValue != testVal)
				{
					errorMessage = "strong user value doesn't match after broadcast";
					return false;
				}
				if (weakUser->myValue != testVal)
				{
					errorMessage = "weak user value doesn't match after broadcast";
					return false;
				}

				//test that after explicit removes, values don't change
				int valueShouldNeverBeStored = testVal + 88;
				basicDelegate.removeStrong(strongUser, &User::handler);
				basicDelegate.removeWeak(weakUser, &User::handler);

				basicDelegate.broadcast(valueShouldNeverBeStored);
				if (strongUser->myValue == valueShouldNeverBeStored)
				{
					errorMessage = "strong user value changed after being removed from delegate";
					return false;
				}
				if (weakUser->myValue == valueShouldNeverBeStored)
				{
					errorMessage = "weak user value changed after being removed from delegate";
					return false;
				}
				//test remove all
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.addWeakObj(weakUser, &User::handler);
				basicDelegate.removeAll(strongUser);
				basicDelegate.removeAll(weakUser);
				basicDelegate.broadcast(88);
				basicDelegate.broadcast(valueShouldNeverBeStored);
				if (strongUser->myValue == valueShouldNeverBeStored)
				{
					errorMessage = "strong user value changed after being removed from delegate";
					return false;
				}
				if (weakUser->myValue == valueShouldNeverBeStored)
				{
					errorMessage = "weak user value changed after being removed from delegate";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// test inheritance subscription
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class TestInheritanceSubscription : public MultiDelegate_UnitTest
		{
			struct Parent : public GameEntity
			{
				void handler(int value) { myValue = value; }
				virtual void virtual_handler(int value) { myValue = value; }

				int myValue = 0;
			};

			struct Child : public Parent
			{
				/** makes the value negative, to prove that virtual function either works or doesn't work*/
				virtual void virtual_handler(int value) override { myValue = -value; }
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Inheritance Subscription";

				//poly morphic pointer
				sp<Parent> childUser = new_sp<Child>();


				MultiDelegate<int> basicDelegate;

				//test child class binding parent class function
				int parentTestValue = 6;
				basicDelegate.addStrongObj(childUser, &Parent::handler);
				basicDelegate.broadcast(parentTestValue);
				if (childUser->myValue != parentTestValue)
				{
					errorMessage = "Something went wrong with binding parent non-virtual function to child class";
					return false;
				}

				//non-polymorphic pointer
				int alternativeSyntaxValue = 44;
				sp<Child> childUserConcrete = new_sp<Child>();
				basicDelegate.addStrongObj(std::static_pointer_cast<Parent>(childUserConcrete), &Parent::handler); //adding this in for demo on syntax
				basicDelegate.broadcast(alternativeSyntaxValue);
				if (childUser->myValue != alternativeSyntaxValue)
				{
					errorMessage = "Something went wrong with binding parent non-virtual function to child class";
					return false;
				}

				//polymorphic bindings
				const int positiveValue = 137;
				MultiDelegate<int> virtualTestDelagate;

				//virtualTestDelagate.addStrongObj(childUser, &Child::virtual_handler); //cannot mis-match parent and child classes, syntax doesn't work to deduce T

				//notice we're binding the parent class function
				virtualTestDelagate.addStrongObj(childUser, &Parent::virtual_handler);
				virtualTestDelagate.broadcast(positiveValue);
				if (childUser->myValue != (-1) *positiveValue)
				{
					if (childUser->myValue == positiveValue)
						errorMessage = "child did not update polymorphically; parent override was called";
					else
						errorMessage = "child did not update polymorphically; parent override was NOT called";
					return false;
				}

				//testing that parent doesn't have weird memory issues, and that the appropriate super class override is called
				sp<Parent> parent = new_sp<Parent>();
				virtualTestDelagate.addStrongObj(parent, &Parent::virtual_handler);

				//if you have a pointer to the child class type, you can bind directly to that function
				virtualTestDelagate.addStrongObj(childUserConcrete, &Child::virtual_handler);

				virtualTestDelagate.broadcast(positiveValue);

				if (childUserConcrete->myValue != (-1) *positiveValue)
				{
					if (childUser->myValue == positiveValue)
						errorMessage = "child did not update polymorphically; parent override was called";
					else
						errorMessage = "child did not update polymorphically; parent override was NOT called";
					return false;
				}

				if (childUserConcrete->myValue == positiveValue)
				{
					errorMessage = "Parent did not have appropriate override called";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///test adding same delegate twice (more ore more) and removals sequentially
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_DelegateOversubscription : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int) { myValue++; }
				int myValue = 0;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Delegate Oversubscription";
				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();
				MultiDelegate<int> basicDelegate;
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.addStrongObj(strongUser, &User::handler);

				basicDelegate.addWeakObj(weakUser, &User::handler);
				basicDelegate.addWeakObj(weakUser, &User::handler);

				basicDelegate.broadcast(0);
				if (strongUser->myValue != 2)
				{
					errorMessage = "strong user value didn't increment twice with 2 subscriptions";
					return false;
				}
				if (weakUser->myValue != 2)
				{
					errorMessage = "weak user value didn't increment twice with 2 subscriptions";
					return false;
				}

				//test remove all only removes 1 subscription
				basicDelegate.removeStrong(strongUser, &User::handler);
				basicDelegate.removeWeak(weakUser, &User::handler);

				basicDelegate.broadcast(0);
				if (strongUser->myValue != 3)
				{
					errorMessage = "strong user value didn't increment after removing addition subscription";
					return false;
				}
				if (weakUser->myValue != 3)
				{
					errorMessage = "weak user value didn't increment after removing addition subscription";
					return false;
				}
				basicDelegate.removeStrong(strongUser, &User::handler);
				basicDelegate.removeWeak(weakUser, &User::handler);

				//test remove all removes all subscriptions (intentionally the strong user)
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.addStrongObj(strongUser, &User::handler);
				basicDelegate.addWeakObj(strongUser, &User::handler);
				basicDelegate.addWeakObj(strongUser, &User::handler);
				basicDelegate.removeAll(strongUser); //should remove all bindings
				basicDelegate.broadcast(0);
				if (strongUser->myValue != 3)
				{
					errorMessage = "incremented after remove-all";
					return false;
				}
				if (weakUser->myValue != 3)
				{
					errorMessage = "incremented after remove-all";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// test Lvalue/const ref bindings
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class TestDelegateParamQualitifers : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int pod, double& ref, const char& cref, float&& rvalue)
				{
					myInt = pod;
					ref = myDouble;
					myChar = cref;
					myFloat = rvalue;
				}
				int myInt = 0;
				const double myDouble = 335; //actually used to set output ref
				char myChar = 'a';
				float myFloat = 0.0f;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Delegate Param Qualifiers";

				const int testInt = 43;
				double outDouble = 0.0f;

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<int, double&, const char&, float&&> complexDelegate;
				complexDelegate.addStrongObj(strongUser, &User::handler);
				complexDelegate.addWeakObj(weakUser, &User::handler);
				complexDelegate.broadcast(testInt, outDouble, 'z', 5.5f);

				sp<User>users[] = { strongUser, weakUser };
				for (sp<User> user : users)
				{
					if (user->myInt != testInt)
					{
						errorMessage = "failed to update user's int from delegate";
						return false;
					}
					if (user->myDouble != outDouble)
					{
						errorMessage = "failed to update out-param double";
						return false;
					}
					if (user->myChar != 'z')
					{
						errorMessage = "failed to update char from const char& param";
						return false;
					}
					if (user->myFloat != 5.5f)
					{
						errorMessage = "failed to update float from float&& param";
						return false;
					}
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///Test passing user types
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class TestCustomUserTypeParams : public MultiDelegate_UnitTest
		{
			struct UserType
			{
				UserType(std::string inval) : myValue(inval) {}
				virtual ~UserType() {} //force this to be non-aggregate by adding a virtual (believe this will work)

				std::string myValue;
			};

			struct User : public GameEntity
			{
				UserType a{ "z" };
				const UserType b{ "BetterUpdateOutParam!" };
				UserType c{ "z" };
				UserType d{ "z" };

				void handler(UserType copy, UserType& ref, const UserType&constref, UserType&&rvalue)
				{
					a = copy;
					ref = b;
					c = constref;
					d = rvalue;
				}
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Custom User Type Params";

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<UserType, UserType&, const UserType&, UserType&&> complexDelegate;
				complexDelegate.addStrongObj(strongUser, &User::handler);
				complexDelegate.addWeakObj(weakUser, &User::handler);


				UserType toCopy("copy");
				UserType outVal("this should get overwritten");

				complexDelegate.broadcast(toCopy, outVal, UserType{ "rvalue pass to const ref" }, UserType{ "rvalue" });

				sp<User>users[] = { strongUser, weakUser };
				for (sp<User> user : users)
				{
					if (user->a.myValue != toCopy.myValue)
					{
						errorMessage = "failed to pass by value";
						return false;
					}
					if (user->b.myValue != outVal.myValue)
					{
						errorMessage = "failed to out ref";
						return false;
					}
					if (user->c.myValue != UserType{ "rvalue pass to const ref" }.myValue)
					{
						errorMessage = "failed to update by const ref";
						return false;
					}
					if (user->d.myValue != UserType{ "rvalue" }.myValue)
					{
						errorMessage = "failed to update by rvalue";
						return false;
					}
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///Test no argument delegate
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_NoArgParams : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler() { hit = true; }
				bool hit = false;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test No Arg Params";

				int testVal = 8;

				sp<User> strongUser = new_sp<User>();
				sp<User> weakUser = new_sp<User>();

				MultiDelegate<> noArgDelegate;
				noArgDelegate.addStrongObj(strongUser, &User::handler);
				noArgDelegate.addWeakObj(weakUser, &User::handler);

				noArgDelegate.broadcast();
				if (!strongUser->hit || !weakUser->hit)
				{
					errorMessage = "failed to update users with no arg delegate";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test removal during broadcast
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_RemovalDuringBroadcast : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int value) { myValue = value; }
				int myValue = 0;

				sp<MultiDelegate<>> sharedNoArgDelegate = nullptr;
				void HandleRemoveWeakDuringBroadcast()
				{
					myValue++;
					sharedNoArgDelegate->removeWeak(sp_this(), &User::HandleRemoveWeakDuringBroadcast);
				}
				void HandleRemoveStrongDuringBroadcast()
				{
					myValue++;
					sharedNoArgDelegate->removeStrong(sp_this(), &User::HandleRemoveStrongDuringBroadcast);
				}
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Removal During Broadcast";

				sp<User> user = new_sp<User>();

				sp<MultiDelegate<>> sharedNoArgDelegate = new_sp<MultiDelegate<>>();
				user->sharedNoArgDelegate = sharedNoArgDelegate;

				//test weak variant
				sharedNoArgDelegate->addWeakObj(user, &User::HandleRemoveWeakDuringBroadcast);
				sharedNoArgDelegate->broadcast(); //this should hit delegate, and it should remove itself
				sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 

				if (user->myValue != 1)
				{
					errorMessage = "increment did not occur only one time";
					return false;
				}

				//test strong variant
				sharedNoArgDelegate->removeAll(user);
				user->myValue = 0;
				sharedNoArgDelegate->addStrongObj(user, &User::HandleRemoveStrongDuringBroadcast);
				sharedNoArgDelegate->broadcast(); //this should hit delegate, and it should remove itself
				sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 
				if (user->myValue != 1)
				{
					errorMessage = "increment did not occur only one time";
					return false;
				}

				//test that all delegates finish firing even when early removals happen
				user->myValue = 0;
				sharedNoArgDelegate->addStrongObj(user, &User::HandleRemoveStrongDuringBroadcast);
				sharedNoArgDelegate->addStrongObj(user, &User::HandleRemoveStrongDuringBroadcast);
				sharedNoArgDelegate->addWeakObj(user, &User::HandleRemoveWeakDuringBroadcast);
				sharedNoArgDelegate->addWeakObj(user, &User::HandleRemoveWeakDuringBroadcast);

				sharedNoArgDelegate->broadcast(); //this should hit delegate, and it should remove itself
				sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 

				if (user->myValue != 4)
				{
					errorMessage = "exactly 4 increments did not occur from exactly 4 subscriptions after broadcasts that remove from the delegates";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test add during broadcast
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_AddDuringBroadcast : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void AddedCallback() { myValue++; }
				int myValue = 0;

				sp<MultiDelegate<>> addDelegate = nullptr;
				void HandleAddDuringBroadcast()
				{
					//over subscribe to test that number of additions matches
					addDelegate->addStrongObj(sp_this(), &User::AddedCallback);
					addDelegate->addStrongObj(sp_this(), &User::AddedCallback);
					addDelegate->addWeakObj(sp_this(), &User::AddedCallback);
					addDelegate->addWeakObj(sp_this(), &User::AddedCallback);
				}
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Add During Broadcast";
				sp<User> user = new_sp<User>();

				sp<MultiDelegate<>> sharedNoArgDelegate = new_sp<MultiDelegate<>>();
				user->addDelegate = sharedNoArgDelegate;

				sharedNoArgDelegate->addWeakObj(user, &User::HandleAddDuringBroadcast);
				sharedNoArgDelegate->broadcast(); //this should not increment anything, but instead add the increment handlers

				if (user->myValue != 0)
				{
					errorMessage = "increment happened during same broadcast as adding the increment handler, delegates added during broadcast should not firing in broadcast ";
					return false;
				}

				sharedNoArgDelegate->broadcast(); //this should not have a callback, cbTest should have removed itself 

				if (user->myValue != 4)
				{
					errorMessage = "increment did not occur for all handlers";
					return true;
				}

				sharedNoArgDelegate->removeAll(user);
				user->myValue = 0;
				sharedNoArgDelegate->broadcast();
				if (user->myValue != 0)
				{
					errorMessage = "remove all did not clear all remaining handlers";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test broadcasting a/the delegate 
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_PassingDelegateAsParam : public MultiDelegate_UnitTest
		{


			struct DelegatePassingObject : public GameEntity
			{
				void handler(MultiDelegate<int>& InDelegate)
				{
					InDelegate.broadcast(correctValue);
				}
				const int correctValue = 99;
			};

			struct SecondCallbackHandler : public GameEntity
			{
				void handler(int inValue) { myValue = inValue; }
				int myValue = 0;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Passing Delegate As Param";

				int testVal = 8;

				sp<DelegatePassingObject> user = new_sp<DelegatePassingObject>();
				sp<SecondCallbackHandler> second = new_sp<SecondCallbackHandler>();

				MultiDelegate<MultiDelegate<int>&> delegatePasser;
				MultiDelegate<int> toPass;

				toPass.addWeakObj(second, &SecondCallbackHandler::handler);
				delegatePasser.addWeakObj(user, &DelegatePassingObject::handler);

				delegatePasser.broadcast(toPass);

				if (second->myValue != user->correctValue)
				{
					errorMessage = "Delegate fire chain did not correctly set correct value";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test Stale Weak Bindings Removed
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_ExpiredWeakBindingsRemoved : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int& outVal)
				{
					outVal = correctValue;
				}
				const int correctValue = 1;
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test Expired Weak Bindings Removed";

				int storageVal;

				MultiDelegate<int&> basicDelegate;
				{
					sp<User> scopedMT = new_sp<User>();
					basicDelegate.addWeakObj(scopedMT, &User::handler);
					basicDelegate.broadcast(storageVal); // should broadcast once

					//just udpate it to anything other than 0
					if (storageVal == 0)
					{
						errorMessage = "failed to update value during broadcast, this is not current main test but the value should change!";
						return false;
					}
				}
				//reset what was done while the object was in scope
				storageVal = 0;

				basicDelegate.broadcast(storageVal);
				if (storageVal != 0)
				{
					errorMessage = "Value should not change after the object leaves scope and is destroyed!";
					return false;
				}

				if (basicDelegate.numStrong() != 0 || basicDelegate.numWeak() != 0)
				{
					errorMessage = "Non-zero subscribers after scope of object was left";
					return false;
				}

				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Test Has Strong Bindings
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Test_NumStrongBindings : public MultiDelegate_UnitTest
		{
			struct User : public GameEntity
			{
				void handler(int& outVal)
				{
					//irrelevant 
				}
			};

			virtual bool runInternal(bool stopOnFail = false) override
			{
				testName = "Test querying is object strong bound";

				MultiDelegate<int&> basicDelegate;
				sp<User> user = new_sp<User>();
				
				basicDelegate.addStrongObj(user, &User::handler);

				if (!basicDelegate.hasBoundStrong(*user.get()))
				{
					errorMessage = "Did not detect that game entity object was bound to delegate";
					return false;
				}

				basicDelegate.removeStrong(user, &User::handler);
				
				if (basicDelegate.hasBoundStrong(*user.get()))
				{
					errorMessage = "User is still registering as bound after removal.";
					return false;
				}
				return true;
			}
		};

		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Container test suite
		/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class DelegateTestSuit : public SA::TestSuite
		{
		public:
			DelegateTestSuit()
			{
				testName = "DELEGATE TEST SUITE";

				//order these in a sensible order where later tests expect earlier tests to pass
				addTest(new_sp<Test_BasicBroadcast>());
				addTest(new_sp<Test_PrivateSubscription>());
				addTest(new_sp<Test_RemoveBindings>());
				addTest(new_sp<TestInheritanceSubscription>());
				addTest(new_sp<Test_DelegateOversubscription>());
				addTest(new_sp<TestDelegateParamQualitifers>());
				addTest(new_sp<TestCustomUserTypeParams>());
				addTest(new_sp<Test_NoArgParams>());
				addTest(new_sp<Test_RemovalDuringBroadcast>());
				addTest(new_sp<Test_AddDuringBroadcast>());
				addTest(new_sp<Test_PassingDelegateAsParam>());
				addTest(new_sp<Test_ExpiredWeakBindingsRemoved>());
				addTest(new_sp<Test_NumStrongBindings>());
			}
		};
	}

	sp<SA::TestSuite> getDelegateTestSuite()
	{
		return new_sp<SA::DelegateTests::DelegateTestSuit>();
	}
}

