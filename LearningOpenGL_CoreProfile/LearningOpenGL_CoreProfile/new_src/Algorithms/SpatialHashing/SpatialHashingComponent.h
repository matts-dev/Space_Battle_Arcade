#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include <list>
#include <memory>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <memory>

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
		const T& element;

		//provided to help shared pointer construction
		GridNode(const T& inElement) : element(inElement) {}
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

		~HashEntry() 
		{
			//this requires that the owning spatial hash have a lifetime greater than any hash entries
			owningGrid.remove(*this);
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
	private:

		inline size_t hash(glm::ivec3 location)
		{
			size_t x = static_cast<size_t>(location.x);
			size_t y = static_cast<size_t>(location.y);
			size_t z = static_cast<size_t>(location.z);

			//pack all values into a single long before hash; this is adhoc bitwise operations that seems non-destructive
			size_t bitvec = x;
			bitvec <<= 32;
			bitvec |= y;
			bitvec ^= (z << 16);

			//hash
			size_t hash = std::hash<size_t>{}(bitvec);

			return hash;
		}

		void hashInsert(std::shared_ptr<GridNode<T>>& gridNode, glm::ivec3 hashLocation)
		{
			using UMMIter = typename decltype(hashMap)::iterator;

			size_t hashVal = hash(hashLocation);

			std::shared_ptr<HashCell<T>> targetCell = nullptr;

			//walk over bucket to find correct cell
			std::pair<UMMIter, UMMIter> bucketRange = hashMap.equal_range(hashVal);
			UMMIter& start = bucketRange.first;
			UMMIter& end = bucketRange.second;
			for (UMMIter bucketIter = start; bucketIter != end; ++bucketIter)
			{
				std::shared_ptr<HashCell<T>>& cell = bucketIter->second;
				if (cell->location == hashLocation)
				{
					targetCell = cell;
					break;
				}
			}
			 
			if (!targetCell)
			{
				std::shared_ptr<HashCell<T>> cell = recyclePool.requestObject();
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

			size_t hashVal = hash(hashLocation);
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
		SpatialHashGrid(const glm::vec4& inGridCellSize) //TODO drop to vec3 if 4th component not actually used
			: gridCellSize(inGridCellSize)
		{
		}
		~SpatialHashGrid()
		{
		}

		/** 
		* attempt add with a transformed bounding box that will be used to figure out cells
		* bounding box has 8 corner vertices, which is why this an array of size 8. 
		*
		* unique_ptr holds an RAII object that will manage cleanup from the spatial hash.
		*/
		std::unique_ptr<HashEntry<T>> insert(const T& obj, const std::array<glm::vec4, 8>& localSpaceOBB)
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
			//        D       |           C    |    A           |       B
			// <----(-3)----(-2)----(-1)-----(0.0)-----(1)-----(2)-----(3)----->
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
			std::unique_ptr<HashEntry<T>> hashResult = std::unique_ptr<HashEntry<T>>( 
				new HashEntry<T>(gridNode, xCellIndices, yCellIndices, zCellIndices,*this)
			);

			//__for cell in every range, add node to spatial hash__
			for (int cellX = xCellIndices.min; cellX <= xCellIndices.max; ++cellX)
			{
				for (int cellY = yCellIndices.min; cellY <= yCellIndices.max; ++cellY)
				{
					for (int cellZ = zCellIndices.min; cellZ <= zCellIndices.max; ++cellZ)
					{
						hashInsert(gridNode, { cellX, cellY, cellZ } );
					}
				}
			}

			//return managed node
			return std::move(hashResult);
		}

		bool remove(const HashEntry<T>& toRemove)
		{
			const Range<int>& xCellIndices = toRemove.xGridCells;
			const Range<int>& yCellIndices = toRemove.yGridCells;
			const Range<int>& zCellIndices = toRemove.zGridCells;

			bool allRemoved = true;
			for (int cellX = xCellIndices.min; cellX <= xCellIndices.max; ++cellX)
			{
				for (int cellY = yCellIndices.min; cellY <= yCellIndices.max; ++cellY)
				{
					for (int cellZ = zCellIndices.min; cellZ <= zCellIndices.max; ++cellZ)
					{
						allRemoved &= hashRemove(toRemove.insertedNode, { cellX, cellY, cellZ });
					}
				}
			}
			return allRemoved;
		}

	public:
		const glm::vec4 gridCellSize;

	private:
		/** Hashmap with buckets for collisions */
		std::unordered_multimap<size_t, std::shared_ptr<HashCell<T>>> hashMap;
		RecyclePool<HashCell<T>> recyclePool;
	};

}





#include "SpatialHashingComponent.inl"
