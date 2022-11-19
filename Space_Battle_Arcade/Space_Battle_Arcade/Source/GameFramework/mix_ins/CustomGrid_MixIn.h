#pragma once
#include <vector>
#include "../SAGameEntity.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "Tools/DataStructures/SATransform.h"
#include <optional>

namespace SA
{
	/** for agnostic abstract storage*/
	struct GridWrapperBase
	{};

	/* concrete storage */
	template <typename ContentType>
	struct GridWrapper : public GridWrapperBase
	{
		GridWrapper(const glm::vec3& gridSize)
			: grid(gridSize)
		{}
		SH::SpatialHashGrid<ContentType> grid;
	};


	class CustomGrid_MixIn
	{
	protected:
		friend class LevelBase;

		/** note: intentionally doesn't return created component to prevent misuse of API (using create instead of get). Use getTypedGrid to get the component created*/
		template<typename ContentType>
		SH::SpatialHashGrid<ContentType>* createTypedGrid(const glm::vec3& gridSize);

		template<typename ContentType>
		void deleteTypedGrid();

	public:
		template<typename ContentType>
		bool hasTypedGrid();

		template<typename ContentType>
		SH::SpatialHashGrid<ContentType>* getTypedGrid();

		/** All operations are funneled through _internalOperation<> and it is nonconst. Const methods
			also need to be funneled through this method. Because create/delete operations are inherently non-const,
			_internalOperation<> must be none const. In order const version of has/getTypedGrid, we use const_cast considering
			all the above. */
		template<typename ContentType>
		const SH::SpatialHashGrid<ContentType>* getTypedGrid() const;

		template<typename ContentType>
		bool hasTypedGrid() const;


	private:
		/** Operations into the single method generated for each grid */
		enum class Op : uint8_t { GET_OP, CREATE_OP, DELETE_OP };

		/** Arguments to be used internally to local static mapping index function*/
		struct InternalArgs
		{
			std::optional<glm::vec3> gridSize;
		};

		template<typename ContentType>
		inline SH::SpatialHashGrid<ContentType>* _internalOperation(Op operation, const InternalArgs& args = {});
	private:
		static size_t nextGridCreationIndex;
		std::vector<sp<GridWrapperBase>> gridArray;
	};



























	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Implementation
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename ContentType>
	SH::SpatialHashGrid<ContentType>* CustomGrid_MixIn::createTypedGrid(const glm::vec3& gridSize)
	{
		static_assert(std::is_base_of<GameEntity, ContentType>::value);
		InternalArgs args;
		args.gridSize = gridSize;

		return _internalOperation<ContentType>(Op::CREATE_OP, args);
	}

	template<typename ContentType>
	void CustomGrid_MixIn::deleteTypedGrid()
	{
		static_assert(std::is_base_of<GameEntity, ContentType>::value);
		_internalOperation<ContentType>(Op::DELETE_OP);
	}

	template<typename ContentType>
	bool CustomGrid_MixIn::hasTypedGrid()
	{
		static_assert(std::is_base_of<GameEntity, ContentType>::value);
		return _internalOperation<ContentType>(Op::GET_OP) != nullptr;
	}

	template<typename ContentType>
	SH::SpatialHashGrid<ContentType>* CustomGrid_MixIn::getTypedGrid()
	{
		static_assert(std::is_base_of<GameEntity, ContentType>::value);
		return _internalOperation<ContentType>(Op::GET_OP);
	}

	template<typename ContentType>
	const SH::SpatialHashGrid<ContentType>* CustomGrid_MixIn::getTypedGrid() const
	{
		SH::SpatialHashGrid<ContentType>* nonConstThis = const_cast<SH::SpatialHashGrid<ContentType>*>(this);
		return nonConstThis->getTypedGrid<ContentType>();
	}

	template<typename ContentType>
	bool CustomGrid_MixIn::hasTypedGrid() const
	{
		SH::SpatialHashGrid<GameEntity>* nonConstThis = const_cast<SH::SpatialHashGrid<ContentType>*>(this);
		return nonConstThis->hasTypedGrid<ContentType>();
	}

	template<typename ContentType>
	inline SH::SpatialHashGrid<ContentType>* CustomGrid_MixIn::_internalOperation(Op operation, const InternalArgs& args)
	{
		//this will create many different functions that automatically know the index of the type
		//operations all live in the same function so that the this_type_index is always the same

		static_assert(std::is_base_of<GameEntity, ContentType>::value);
		static size_t this_type_index = nextGridCreationIndex++; //first time this is called, it gets a new index for fast compile time lookup

		if (gridArray.size() < this_type_index + 1)
		{
			gridArray.resize(this_type_index + 1, nullptr);
		}

		SH::SpatialHashGrid<ContentType>* ret = nullptr;

		if (operation == Op::GET_OP)
		{
			//static cast is safe since we're using index look up as a proxy for RTTI
			GridWrapper<ContentType>* wrapper = static_cast<GridWrapper<ContentType>*>(gridArray[this_type_index].get());
			ret = wrapper ? &wrapper->grid : nullptr;
		}
		else if (operation == Op::CREATE_OP)
		{
			if (gridArray[this_type_index] == nullptr)
			{
				gridArray[this_type_index] = new_sp<GridWrapper<ContentType>>(args.gridSize.value());
				GridWrapper<ContentType>* wrapper = static_cast<GridWrapper<ContentType>*>(gridArray[this_type_index].get());
				assert(wrapper);
				ret = static_cast<SH::SpatialHashGrid<ContentType>*>(&wrapper->grid);
			}
			else
			{
				log("custom grid mixin", LogLevel::LOG_WARNING, "attempting to create a custom spatial hash grid that already exists");
				assert(false);
			}
		}
		else if (operation == Op::DELETE_OP)
		{
			if (sp<GridWrapperBase>& gridWrapper = gridArray[this_type_index])
			{
				gridArray[this_type_index] = nullptr;
			}
			else
			{
				log("custom grid mixin", LogLevel::LOG_WARNING, "deleting spatial hash grid that doesn't exist");
				assert(false);
			}
		}
		return ret;
	}

}