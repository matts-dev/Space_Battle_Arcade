#pragma once


#include <vector>

/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// usage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include<iostream>
namespace
{
	std::vector<int> items = { 0,1,2,3,4,5,6,7,8,9,10 };
	void tick(float dt_sec)
	{
		static AmortizeLoopTool AmortizeLoopTool; //don't actually use local statics as they have thread safety overhead in cpp11+
		//AmortizeLoopTool.updateTick(items); //NOTE: this is't necssary if you use "UpdateStart" within loop.

		//need to re-query stop each time if you're going to modify array within loop
		//for (size_t i = AmortizeLoopTool.getStartIdx(); i < AmortizeLoopTool.getStopIdxSafe(items); ++i){
		for (size_t i = AmortizeLoopTool.UpdateStart(); i < AmortizeLoopTool.getStopIdxSafe(items); ++i){
			std::cout << items[i] << std::endl;
		}
		std::cout << "end tick" << std::endl;
	}

	void true_main(){
		for (size_t i = 0; i < 5; ++i){
			tick(0.01f);
		}
	}
}
int main()
{
	true_main();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// output
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
0
1
2
3
end tick
4
5
6
7
end tick
8
9
10
end tick
0
1
2
3
end tick
0
1
2
3
end tick

*/
namespace SA
{
	class AmortizeLoopTool
	{
	public:
		size_t chunkSize = 4; //number of items in an amortization tick
	private:
		size_t tickStartIdx = 0;
		size_t amortizedIdx = 0; //a moving index that wraps around
	public: //API
		size_t getStartIdx() { return tickStartIdx; }
		size_t getStopIdxNonsafe() { return tickStartIdx + chunkSize; }
		template<typename T>
		inline size_t getStopIdxSafe(const std::vector<T>& arr)
		{
			size_t containerSize = arr.size();
			size_t rawStopIdx = this->getStopIdxNonsafe();
			return containerSize < rawStopIdx ? containerSize : rawStopIdx;
		}

		template<typename T>
		inline size_t updateStart(const std::vector<T>& arr)
		{
			// every tick handle X number of tests (defined by chunkSize);
			// if we multiply the amortized tick idx by chunk size number, we get the start idx for the chunk in an infinite sized array
			// since we're using a fixed size array, we could use modulus to trim this down into the range of our array
			// but consider array size 7, with chunk size of 3 items per tick. the chunk size does not start back at 0 using modulus
			// 0 = [0-2], 1 = [3-5], 2=[6,7,8], 3=[9=>2,3,4]
			// ie (3 * 3) % 7 = 2; so we need logic to reset once out of bounds; which means simple modulus isn't going to work great
			this->tickStartIdx = amortizedIdx * chunkSize; //spot in infinite sized array


			if (tickStartIdx > arr.size())	//wrap around with branch since modulus (as pointed out above) isn't great.
			{
				this->amortizedIdx = 0;
				this->tickStartIdx = 0;
			}

			++this->amortizedIdx; //apply update after we get start idx so that first tick is at 0

			return tickStartIdx;
		}
	};
}
