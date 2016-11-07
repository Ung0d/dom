/*
* This file is part of the dom-library - an implementation of the Entity-Component-System Pattern.
* Copyright (C) 2016 Felix Becker - fb132550@uni-greifswald.de
*
* This software is provided 'as-is', without any express or
* implied warranty. In no event will the authors be held
* liable for any damages arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute
* it freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented;
*    you must not claim that you wrote the original software.
*    If you use this software in a product, an acknowledgment
*    in the product documentation would be appreciated but
*    is not required.
*
* 2. Altered source versions must be plainly marked as such,
*    and must not be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any
*    source distribution.
*/

#ifndef DOM_CHUNKED_ARRAY_H
#define DOM_CHUNKED_ARRAY_H

#include <queue>
#include <vector>
#include <memory>
#include <functional>

namespace dom
{
    struct ElementHandle
    {
        unsigned short block;
        unsigned short index;
    };

    class BaseChunkedArray {};

    /**
    * \brief A datastructure that stores its elements in semi-continous memory.
    * Elements are arranged in memory-blocks of a fixed size. The datastructure attempts
    * to place that memory blocks close together.
    * Add and destroy operations are cheap and wont require reallocation.
    */
    template<typename T, std::size_t BLOCK_SIZE = 8192>
    class ChunkedArray : public BaseChunkedArray
    {
    private:
        class MemoryBlock;

        std::allocator<T> alloc;
        std::vector< MemoryBlock > mBlocks;
        std::queue<ElementHandle> mFreeSlots;

    public:
        /** \brief Constructs a new array with a single block allocated. */
        ChunkedArray();

        /**
        * \brief Adds a new element and constructs it in place. This may
        * triggers the creation of a new block.
        */
        template<typename ...PARAM>
        ElementHandle add(PARAM&&... param);

        /**
        * \brief Accesses an element through an ElementHandle.
        */
        T& get(ElementHandle h);
        const T& get(ElementHandle h) const;

        /**
        * \brief Destroys an element through an ElementHandle.
        */
        void destroy(ElementHandle h);

        /** \brief Returns the number of memory blocks currently allocated. */
        std::size_t blockCount() const;

        ~ChunkedArray();

    private:
        class MemoryBlock
        {
        friend class ChunkedArray<T, BLOCK_SIZE>;
        private:
            std::size_t contentCount;
            T* ptr;

        public:
            MemoryBlock() : contentCount(0), ptr(nullptr) {}
        };
    };



    template<typename T, std::size_t BLOCK_SIZE>
    ChunkedArray<T, BLOCK_SIZE>::ChunkedArray()
    {
        mBlocks.emplace_back();
        mBlocks.back().ptr = alloc.allocate(BLOCK_SIZE);
    }

    template<typename T, std::size_t BLOCK_SIZE>
    template<typename ...PARAM>
    ElementHandle ChunkedArray<T, BLOCK_SIZE>::add(PARAM&&... param)
    {
        ElementHandle h;
        if (mFreeSlots.size() > 0) //reuse a previously abadoned slot
        {
            h = mFreeSlots.front();
            mFreeSlots.pop();
        }
        else if (mBlocks.back().contentCount >= BLOCK_SIZE) //need to create a new block
        {
            T* hint = mBlocks.back().ptr + BLOCK_SIZE;
            mBlocks.emplace_back();
            mBlocks.back().ptr = alloc.allocate(BLOCK_SIZE, hint); //allocate new memory possibly near the existing blocks
            h.block = mBlocks.size() -1;
            h.index = 0;
        }
        else
        {
            h.block = mBlocks.size() -1;
            h.index = mBlocks.back().contentCount;
        }
        ++(mBlocks[h.block].contentCount);
        alloc.construct( mBlocks[h.block].ptr + h.index,
                         std::forward<PARAM>(param)... );  //construct component C in place
        return h;
    }

    template<typename T, std::size_t BLOCK_SIZE>
    T& ChunkedArray<T, BLOCK_SIZE>::get(ElementHandle h)
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE>
    const T& ChunkedArray<T, BLOCK_SIZE>::get(ElementHandle h) const
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE>
    void ChunkedArray<T, BLOCK_SIZE>::destroy(ElementHandle h)
    {
        --(mBlocks[h.block].contentCount);
        alloc.destroy( mBlocks[h.block].ptr + h.index);
        mFreeSlots.push(h);
    }

    template<typename T, std::size_t BLOCK_SIZE>
    std::size_t ChunkedArray<T, BLOCK_SIZE>::blockCount() const
    {
        return mBlocks.size();
    }

    template<typename T, std::size_t BLOCK_SIZE>
    ChunkedArray<T, BLOCK_SIZE>::~ChunkedArray()
    {
        for (const auto& block : mBlocks)
        {
            alloc.deallocate(block.ptr, BLOCK_SIZE);
        }
    }
}

#endif // DOM_CHUNKED_ARRAY_H
