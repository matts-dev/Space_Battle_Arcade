#pragma once

#include<set>
#include<unordered_set>

namespace SA
{

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Iterable hash set proides O(1) queries, and O(n) traversal.
	// This is achived by managing 2 data structures underneath. The operation complexities are 
	//
	//								 hash_set		tree_set
	// insertion: o(log(n))		  max( o(1*)		o(log(n))
	// removal:   o(log(n))		  max( o(1*)		o(log(n))
	// contains:  o(1*)				   o(1*)
	// iterate	  o(n)			  min( o(n + m)     o(n)    )
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class IterableHashSet
	{
	public:
		
		/** O(1*) */
		bool contains(const T& item) { return hashSet.find(item) != hashSet.end(); }

		/** O(log(n)) */
		void insert(const T& item)
		{
			hashSet.insert(item);
			treeSet.insert(item);
		}

		/** O(log(n)) */
		void remove(const T& item)
		{
			hashSet.erase(item);
			treeSet.erase(item);
		}

		void clear()
		{
			treeSet.clear();
			hashSet.clear();
		}

	private:
		std::set<T> treeSet;
		std::unordered_set<T> hashSet;

	public: //for-each walk O(n)
		typename decltype(treeSet)::iterator begin() { return treeSet.begin(); }
		typename decltype(treeSet)::const_iterator begin() const { return treeSet.begin(); }
		typename decltype(treeSet)::iterator end() { return treeSet.end(); }
		typename decltype(treeSet)::const_iterator end() const { return treeSet.end(); }
	};

}