#pragma once

#define LOG_LIFETIME_ERRORS 1

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include <iterator>
#include <list>
#include <memory>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <cstdint>

namespace SH
{
	struct Transform
	{
		glm::vec3 position = { 0, 0, 0 };
		glm::quat rotQuat; 
		glm::vec3 scale = { 1, 1, 1 };

		glm::mat4 getModelMatrix()
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = model * glm::toMat4(rotQuat);
			model = glm::scale(model, scale);
			return model;
		}
	};

	//forward declarations
	template<typename T>
	class SpatialHashGrid;

	class RemoveCopies
	{
	public:
		RemoveCopies() = default;
		RemoveCopies(const RemoveCopies& copy) = delete;
		RemoveCopies& operator=(const RemoveCopies& copy) = delete;
	};

	class RemoveMoves
	{
	public:
		RemoveMoves() = default;
		RemoveMoves(RemoveMoves&& move) = delete;
		RemoveMoves& operator=(RemoveMoves&& move) = delete;
	};

	/*
		Benchmarks
		
		Use single threaded boost shared pointers instead of std shared pointers to avoid atomic reference count increments
			results: not tested

		Reduce code duplication by creating inline iterate function that accepts lambda to apply to each cell
			-requires scenario where single object uses multiple cells
			results: not tested
		
		Acceleration structure within lookup functions to prevent adding duplicates
			-requires scenario where single object uses multiple cells
			results: not tested
		
	*/



	//things I would like to support:
	// *make it easy for an object to have multiple grid locations
	// *make access to grid fast and single threaded, and not rely on shared_ptr's reference blocks
	// *support rotating spatial hash grid? this would allow sub-grids (internal vehicles?)but may be hard because will need to iterate over all objects
	// x*support spatial hashes in different vector spaces

	//user puts object into a grid node, from the spatial hash map
	//grid node manages inserting and removing from spatial hash, to prevent leaks
	//this way grid node can be fast and not need shared ptr threadsafe reference count lookup
	//grid can also hold a reference to its owning object

	//testcases to try
	//check to make sure negative number region still correctly works
	//grid cells that are not the same size? thing long rectangles?

	template<typename T>
	class RecyclePool : public RemoveCopies , public RemoveMoves
	{
	public:
		std::shared_ptr<T> requestObject()
		{
			if (freeList.size() > 0)
			{
				auto freeObj = freeList.back();
				freeList.pop_back();
				return freeObj;
			}
			else
			{
				//Requires objects have no-arg constructors, otherwise needs to provide interface
				//to configure objects that come from the recycled pool
				return std::make_shared<T>();
			}
		}

		//put obj back into pool for next request
		void releaseObject(std::shared_ptr<T> obj)
		{
			freeList.push_back(obj);
		}
	private:
		std::list<std::shared_ptr<T>> freeList;
	};

	template<typename T>
	struct Range
	{
		T min;
		T max;
	};

	template<typename T>
	struct GridNode
	{
		T& element;

		//provided to help shared pointer construction
		GridNode(T& inElement) : element(inElement) {}
	};

	/**
	 * The return from hashing an object into the spatial hash. RTTI manages grid cleanup.
	 * note: there may be huge perf speed up may come from using boost's single threaded local_shared_ptr to avoid thread-safe reference counting
	 */
	template<typename T>
	struct HashEntry final : public RemoveCopies, public RemoveMoves
	{
		/** Inserted node may be found in one or more grid cell; it wraps the object*/
		const std::shared_ptr<GridNode<T>> insertedNode; 
		const Range<int> xGridCells;
		const Range<int> yGridCells;
		const Range<int> zGridCells;
		SpatialHashGrid<T>& owningGrid;
		bool gridValid = true;

		~HashEntry() 
		{
			if (gridValid)
			{
				owningGrid.remove(*this);
			}
#ifdef LOG_LIFETIME_ERRORS
			else
			{
				std::cerr << "Hash Entry outlived spatial hash; this is probably " << std::endl;
			}
#endif // LOG_LIFETIME_ERRORS
		}
		
	private:
		friend SpatialHashGrid<T>;

		HashEntry(
			const std::shared_ptr<GridNode<T>>& inInsertedNode,
			const Range<int>& inXGridCells,const Range<int>& inYGridCells,const Range<int>& inZGridCells,
			SpatialHashGrid<T>& inOwningGrid
		) :
			insertedNode(inInsertedNode),
			xGridCells(inXGridCells), yGridCells(inYGridCells), zGridCells(inZGridCells), 
			owningGrid(inOwningGrid)
		{ }
	};

	template<typename T>
	struct HashCell : public RemoveCopies, public RemoveMoves
	{
		glm::ivec3 location;
		std::list<std::shared_ptr<GridNode<T>>> nodeBucket;
	};



	template<typename T>
	class SpatialHashGrid : public RemoveCopies, public RemoveMoves
	{

///////////////////////////////////////////////////////////////////////////////////////
// Helper macros for guaranteed inlining and removing code duplication
///////////////////////////////////////////////////////////////////////////////////////
#define BEGIN_FOR_EVERY_CELL(xCellIndices, yCellIndices, zCellIndices) \
for (int cellX = xCellIndices.min; cellX < xCellIndices.max; ++cellX)\
{\
	for (int cellY = yCellIndices.min; cellY < yCellIndices.max; ++cellY)\
	{\
		for (int cellZ = zCellIndices.min; cellZ < zCellIndices.max; ++cellZ)\
		{

#define END_FOR_EVERY_CELL \
		}\
	}\
}\
///////////////////////////////////////////////////////////////////////////////////////
	private:

		inline uint64_t hash(glm::ivec3 location)
		{
			uint32_t x = static_cast<uint32_t>(location.x);
			uint32_t y = static_cast<uint32_t>(location.y);
			uint32_t z = static_cast<uint32_t>(location.z);

			//pack all values into a single long before hash; this is adhoc bitwise operations that seems non-destructive
			uint64_t bitvec = x;
			bitvec = bitvec << 32ul;
			bitvec |= y;
			bitvec ^= (z << 16ul);

			//hash
			uint64_t hash = std::hash<uint64_t>{}(bitvec);

			return hash;
		}

		void hashInsert(std::shared_ptr<GridNode<T>>& gridNode, glm::ivec3 hashLocation)
		{
			using UMMIter = typename decltype(hashMap)::iterator;

			uint64_t hashVal = hash(hashLocation);

			std::shared_ptr<HashCell<T>> targetCell = nullptr;

			int debug_bucketsForHash = 0;
			//walk over bucket to find correct cell
			std::pair<UMMIter, UMMIter> bucketRange = hashMap.equal_range(hashVal);
			UMMIter& start = bucketRange.first;
			UMMIter& end = bucketRange.second;
			for (UMMIter bucketIter = start; bucketIter != end; ++bucketIter)
			{
				std::shared_ptr<HashCell<T>>& cell = bucketIter->second;
				++debug_bucketsForHash;
				uint64_t debug_cellHash = hash(cell->location);
				if (cell->location == hashLocation)
				{
					targetCell = cell;
					break;
				}
			}
			 
			if (!targetCell)
			{
				std::shared_ptr<HashCell<T>> cell = recyclePool.requestObject(); //TODO this may not be necessary because of the way HashCell DTOR works -- but might can recycle grid nodes?
				//configure cell
				cell->location = hashLocation;
				cell->nodeBucket.clear();
				targetCell = cell;

				//insert cell
				hashMap.emplace(hashVal, cell);
			}

			//now cell is guaranteed to exist
			targetCell->nodeBucket.push_back(gridNode);
		}

		bool hashRemove(const std::shared_ptr<GridNode<T>>& gridNode, glm::ivec3 hashLocation)
		{
			using UMMIter = typename decltype(hashMap)::iterator;
			bool bFoundRemove;

			uint64_t hashVal = hash(hashLocation);
			std::pair<UMMIter, UMMIter> bucketRange = hashMap.equal_range(hashVal);
			UMMIter& start = bucketRange.first;
			UMMIter& end = bucketRange.second;
			UMMIter targetCell_i = end;
			std::shared_ptr<HashCell<T>> targetCell = nullptr;

			for (UMMIter bucketIter = start; bucketIter != end; ++bucketIter)
			{
				std::shared_ptr<HashCell<T>>& cell = bucketIter->second;
				if (cell->location == hashLocation)
				{
					targetCell = cell;
					targetCell_i = bucketIter;
					break;
				}
			}
			//removals should always be associated with a present cell!
			assert(targetCell);

			if (targetCell)
			{
				using listIter = typename decltype(targetCell->nodeBucket)::iterator;
				listIter node_i = targetCell->nodeBucket.begin();

				//note: bucket is a list of shared ptrs
				listIter removeTarget = targetCell->nodeBucket.end();
				while (node_i != targetCell->nodeBucket.end())
				{
					//these two should represent the same obj; therefore the same address
					if (*node_i == gridNode)
					{
						removeTarget = node_i;
						break;
					}
					++node_i;
				}

				bFoundRemove = removeTarget != targetCell->nodeBucket.end();
				assert(bFoundRemove);
				if (bFoundRemove)
				{
					targetCell->nodeBucket.erase(removeTarget);
				}

				if(targetCell->nodeBucket.size() == 0)
				{
					//remove and recycle cell
					hashMap.erase(targetCell_i);
				}
			}
			return bFoundRemove;
		}

	public:
		SpatialHashGrid(const glm::vec4& inGridCellSize, std::size_t estimatedNumCells = 10000) //TODO drop to vec3 if 4th component not actually used
			: gridCellSize(inGridCellSize)
		{
			hashMap.reserve(estimatedNumCells);
		}
		~SpatialHashGrid()
		{
			//proper usage means that the spatial hash always outlives its entries
			//but for a simple API, this isn't enforced. The cost is a slow dtor operation of the spatial hash
			if (validEntries.size() > 0) //O(1)
			{
#ifdef LOG_LIFETIME_ERRORS
				std::cerr << "WARNING: spatial hash was outlived by its entries; this is likely a design issue" << std::endl;
				std::cerr << "walking distance " << std::distance(validEntries.begin(), validEntries.end())  << " to invalidate entries" << std::endl;
#endif // LOG_LIFETIME_ERRORS
				for (HashEntry<T>* entry : validEntries)
				{
					entry->gridValid = false;
				}
			}
		}

		/** 
		* attempt add with a transformed bounding box that will be used to figure out cells
		* bounding box has 8 corner vertices, which is why this an array of size 8. 
		*
		* unique_ptr holds an RAII object that will manage cleanup from the spatial hash.
		*/
		std::unique_ptr<HashEntry<T>> insert(T& obj, const std::array<glm::vec4, 8>& localSpaceOBB)
		{
			//__project points onto grid cell axes__
			Range<float> xProjRange, yProjRange,zProjRange;
			xProjRange.min = yProjRange.min = zProjRange.min = std::numeric_limits<float>::infinity();
			xProjRange.max = yProjRange.max = zProjRange.max = -std::numeric_limits<float>::infinity();

			//hopefully this will loop unroll
			const size_t size = localSpaceOBB.size();
			for (size_t vert = 0; vert < size; ++vert)
			{
				//when using the true x,y,z axes for cells, projections are just taking components -- dot product is overkill and adds unnecessary overhead
				//eg (7, 8, 9) projected onto the x-axis (1,0,0) is  7*1 + 8*0 + 9*0 = 7; so the dot product is not unneccesary overhead in these scenarios
				const glm::vec4& vertex = localSpaceOBB[vert];
				xProjRange.min = xProjRange.min > vertex.x ? vertex.x : xProjRange.min;
				xProjRange.max = xProjRange.max < vertex.x ? vertex.x : xProjRange.max;

				yProjRange.min = yProjRange.min > vertex.y ? vertex.y : yProjRange.min;
				yProjRange.max = yProjRange.max < vertex.y ? vertex.y : yProjRange.max;

				zProjRange.min = zProjRange.min > vertex.z ? vertex.z : zProjRange.min;
				zProjRange.max = zProjRange.max < vertex.z ? vertex.z : zProjRange.max;
			}

			// __calculate cells overlapped in each grid axis__
			//                |                |                |
			//        D       |       C        |        A       |       B
			// <----(-3)----(-2)----(-1)-----(0.0)-----(1)-----(2)-----(3)----->
			//                |                |                |
			//                |                |                |
			// A = 0.5, B = 3, C = -0.5, D = -3
			// cell size = 2.0
			//
			//mapping into cells is done by the following mapping, use to the visual reference to see why these are needed
			// cell index mapping       MIN      MAX
			// A = 0.5  /2.0  = 0.25    => 0      =>1
			// B = 3.0  /2.0  = 1.5     => 1      =>2
			// C = -0.5 /2.0  = -0.25   => -1     =>0
			// D = -3.0 /2.0  = -1.5    => -2     =>-1
			//using ranges of indexes to avoid using dynamic allocation structures that will use heap
			Range<int> xCellIndices, yCellIndices, zCellIndices;
			float startX = std::floor(xProjRange.min / gridCellSize.x);
			float startY = std::floor(yProjRange.min / gridCellSize.y);
			float startZ = std::floor(zProjRange.min / gridCellSize.z);

			//increment method can be used to find end* to avoid divides, but requires branches
			float endX = std::ceil(xProjRange.max / gridCellSize.x);
			float endY = std::ceil(yProjRange.max / gridCellSize.y);
			float endZ = std::ceil(zProjRange.max / gridCellSize.z);

			//float indices should be pretty close to values, but may have suffered from float imprecision so round them
			//rather than just casting to int because casting will map 0.99 => to 0, (should map to index 1)
			xCellIndices.min = static_cast<int>(std::lroundf(startX));
			yCellIndices.min = static_cast<int>(std::lroundf(startY));
			zCellIndices.min = static_cast<int>(std::lroundf(startZ));

			xCellIndices.max = static_cast<int>(std::lroundf(endX));
			yCellIndices.max = static_cast<int>(std::lroundf(endY));
			zCellIndices.max = static_cast<int>(std::lroundf(endZ));


			std::shared_ptr<GridNode<T>> gridNode = std::make_shared< GridNode<T> >(obj);

			//the following doesn't use std::make_unique due to complexities around friending
			//std::make_unique since the constructor is private; it is far simpler to do it this way
			std::unique_ptr<HashEntry<T>> hashEntry = std::unique_ptr<HashEntry<T>>( 
				new HashEntry<T>(gridNode, xCellIndices, yCellIndices, zCellIndices,*this)
			);

			//__for cell in every range, add node to spatial hash__
			//for (int cellX = xCellIndices.min; cellX <= xCellIndices.max; ++cellX)
			//{
			//	for (int cellY = yCellIndices.min; cellY <= yCellIndices.max; ++cellY)
			//	{
			//		for (int cellZ = zCellIndices.min; cellZ <= zCellIndices.max; ++cellZ)
			//		{
					BEGIN_FOR_EVERY_CELL(xCellIndices, yCellIndices, zCellIndices)
					hashInsert(gridNode, { cellX, cellY, cellZ });
					END_FOR_EVERY_CELL
			//		}
			//	}
			//}

			validEntries.insert(hashEntry.get());
			return std::move(hashEntry);
		}

		void lookupNodesInCells(const SH::HashEntry<T>& cellSource, std::vector<std::shared_ptr<SH::GridNode<T>>>& outNodes, bool filterOutSource = true)
		{
			//TODO user lookup cells function to reduce code duplication?
			using UMMIter = typename decltype(hashMap)::iterator;

			std::unordered_set<GridNode<T>*> previouslyAdded; //dtor is O(n) for unique insertions

			//for (int cellX = cellSource.xGridCells.min; cellX <= cellSource.xGridCells.max; ++cellX)
			//{
			//	for (int cellY = cellSource.yGridCells.min; cellY <= cellSource.yGridCells.max; ++cellY)
			//	{
			//		for (int cellZ = cellSource.zGridCells.min; cellZ <= cellSource.zGridCells.max; ++cellZ)
			//		{
			BEGIN_FOR_EVERY_CELL(cellSource.xGridCells, cellSource.yGridCells, cellSource.zGridCells)
						glm::ivec3 hashLocation(cellX, cellY, cellZ);
						uint64_t hashVal = hash(hashLocation);
						std::pair<UMMIter, UMMIter> bucketRange = hashMap.equal_range(hashVal); //gets start_end iter pair
						int debug_bucketsForHash = 0;

						//look over bucket of cells (ideally this will be a small number)
						for (UMMIter bucketIter = bucketRange.first; bucketIter != bucketRange.second; ++bucketIter)
						{
							++debug_bucketsForHash;
							std::shared_ptr<HashCell<T>>& cell = bucketIter->second;

							//make sure we're not dealing with a cell that is hash collision 
							if (cell->location == hashLocation)
							{
								for (std::shared_ptr <GridNode<T>>& node : cell->nodeBucket)
								{
									bool bFilterFail = filterOutSource && (&node->element == &cellSource.insertedNode->element);

									//make sure this node hasn't already been found and that self-filter didn't fail
									if (previouslyAdded.find(node.get()) == previouslyAdded.end() && !bFilterFail)
									{
										previouslyAdded.insert(node.get());
										outNodes.push_back(node);
									}
								}
							}
						}
			END_FOR_EVERY_CELL
			//		}
			//	}
			//}
		}

		//inline void lookupCellsForEntry(const SH::HashEntry<T>& cellSource, std::vector<std::shared_ptr<const SH::HashCell<T>>>& outCells) const
		//{
		//	lookupCellsForEntry_Internal(cellSource, outCells);
		//}

		//TODO: this should return const refs to hash cells if it is going to be in the public API
		//TODO: update this either use a macro for cell iteration or have a inline helper function that returns list of cells for hash and use that in other functions to avoid code duplication
		inline void lookupCellsForEntry(const SH::HashEntry<T>& cellSource, std::vector<std::shared_ptr<SH::HashCell<T>>>& outCells)
		{
			using UMMIter = typename decltype(hashMap)::iterator;

			//for (int cellX = cellSource.xGridCells.min; cellX <= cellSource.xGridCells.max; ++cellX)
			//{
			//	for (int cellY = cellSource.yGridCells.min; cellY <= cellSource.yGridCells.max; ++cellY)
			//	{
			//		for (int cellZ = cellSource.zGridCells.min; cellZ <= cellSource.zGridCells.max; ++cellZ)
			//		{
			BEGIN_FOR_EVERY_CELL(cellSource.xGridCells, cellSource.yGridCells, cellSource.zGridCells)
						glm::ivec3 hashLocation(cellX, cellY, cellZ);
						uint64_t hashVal = hash(hashLocation);
						std::pair<UMMIter, UMMIter> bucketRange = hashMap.equal_range(hashVal); //gets start_end iter pair

						//look over bucket of cells (ideally this will be a small number)
						for (UMMIter bucketIter = bucketRange.first; bucketIter != bucketRange.second; ++bucketIter)
						{
							std::shared_ptr<HashCell<T>>& cell = bucketIter->second;

							//make sure we're not dealing with a cell that is hash collision 
							if (cell->location == hashLocation)
							{
								outCells.push_back(cell);
							}
						}
			END_FOR_EVERY_CELL
			//		}
			//	}
			//}
		}

		void logDebugInformation()
		{
			std::cout << "bucket count:" << hashMap.bucket_count() << std::endl;
		}

	private:
		friend HashEntry<T>::~HashEntry();
		bool remove(HashEntry<T>& toRemove)
		{
			const Range<int>& xCellIndices = toRemove.xGridCells;
			const Range<int>& yCellIndices = toRemove.yGridCells;
			const Range<int>& zCellIndices = toRemove.zGridCells;

			bool allRemoved = true;
			//for (int cellX = xCellIndices.min; cellX <= xCellIndices.max; ++cellX)
			//{
			//	for (int cellY = yCellIndices.min; cellY <= yCellIndices.max; ++cellY)
			//	{
			//		for (int cellZ = zCellIndices.min; cellZ <= zCellIndices.max; ++cellZ)
			//		{
			BEGIN_FOR_EVERY_CELL(xCellIndices, yCellIndices, zCellIndices)
						allRemoved &= hashRemove(toRemove.insertedNode, { cellX, cellY, cellZ });
			END_FOR_EVERY_CELL
			//		}
			//	}
			//}

			auto iter = validEntries.find(&toRemove);
			assert(iter != validEntries.end());
			validEntries.erase(iter);
			
			return allRemoved;
		}

	public:
		const glm::vec4 gridCellSize;

	private:
		/** Hashmap with buckets for collisions */
		std::unordered_multimap<uint64_t, std::shared_ptr<HashCell<T>>> hashMap;
		/** 
		 * Valid hash entries for invalidation if spatial hash is destroyed before the entries are released. Ideally this shouldn't happen, but in order to provide the simplest interface the behavior is allowed.
		 * The HashEntries need to be unique pointers so that proper resource clean up occurs; having entries as unique pointers means that we cannot have this use smart pointer to the entries
		 * While this is a probably an anti-pattern, the construction of HashEntries extremly tightly coupled to the spatial hash; client usage ineractions with entries is so restricted that this usage is okay. 
		 * TODO revise this comment
		 */
		std::unordered_set<HashEntry<T>*> validEntries;
		RecyclePool<HashCell<T>> recyclePool;
	};

}





#include "SpatialHashingComponent.inl"
