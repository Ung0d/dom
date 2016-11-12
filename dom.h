/*
* This is the dom-library - an implementation of the Entity-Component-System Pattern.
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

#ifndef DOM_LIBRARY_H
#define DOM_LIBRARY_H

#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include <bitset>
#include <array>
#include <unordered_map>
#include <stdexcept>

namespace dom
{
    template <typename CINDEX, CINDEX COMP_TOTAL> class Universe;

    /** \brief A handle object to access an element in the ChunkedArray.
    * Works with double indexing (block,index). */
    struct ChunkedArrayHandle
    {
        unsigned short block;
        unsigned short index;
    };

    class BaseChunkedArray
    {
    public:  virtual void destroy(ChunkedArrayHandle h) = 0;
    };

    /**
    * \brief A datastructure that stores its elements in semi-continous memory.
    * Elements are arranged in memory-blocks of a fixed size. The datastructure attempts
    * to place that memory blocks close together.
    * Add and destroy operations are cheap and wont require reallocation.
    * Template types:
    * T ... Type of the object to be stored
    * BLOCK_SIZE ... number of objects with type T, that can be stored in a continous block
    * REUSE_C ... minimum stack-size for free slots. Choose value > 0 only if you want to
    * avoid, that single slots are reused too often.
    */
    template<typename T, std::size_t BLOCK_SIZE = 8192, std::size_t REUSE_C = 0>
    class ChunkedArray : public BaseChunkedArray
    {
    private:
        class MemoryBlock;

        std::allocator<T> alloc;
        std::vector< MemoryBlock > mBlocks;
        std::queue<ChunkedArrayHandle> mFreeSlots;

    public:
        /** \brief Constructs a new array with a single block allocated. */
        ChunkedArray();

        /**
        * \brief Adds a new element and constructs it in place. This may
        * triggers the creation of a new block.
        */
        template<typename ...PARAM>
        ChunkedArrayHandle add(PARAM&&... param);

        /**
        * \brief Accesses an element through an ChunkedArrayHandle.
        */
        T& get(ChunkedArrayHandle h);
        const T& get(ChunkedArrayHandle h) const;

        /**
        * \brief Destroys an element through an ChunkedArrayHandle.
        */
        virtual void destroy(ChunkedArrayHandle h) override;

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


    /**
    * \brief A base class for all component base structs.
    * CINDEX is a index type for components, rather small. COMP_TOTAL is the total number of components allowed.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct ComponentTraitsBase
    {
    protected:
        /** \brief Returns a new, unique ID for a component-type, but doesnt give
        * more than COMP_TOTAL different IDs. */
        static CINDEX newID();
    };

    /**
    * \brief A traits class for a specific component type C.
    * Provides a unique ID for that type.
    * Throws an exception, if a new ID is requested, when there are already
    * COMP_TOTAL different component-types registered.
    */
    template<typename C, typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct ComponentTraits : public ComponentTraitsBase<CINDEX, COMP_TOTAL>
    {
    public:
        static CINDEX getID();
    };

    /**
    * \brief An error thrown by ComponentTraitsBase, if there are more then
    * COMP_TOTAL ids requested.
    */
    struct ComponentCountError : public std::runtime_error
    {
        ComponentCountError();
    };



    using EntityChunkedArrayHandle = ChunkedArrayHandle;

    /**
    * \brief A handle class acts as a pointer to an entity.
    * Entities are never accessed directly, always through handles.
    * The entity-data itself in stored in the Universe in
    * semi-continous storage. The universe pointer is bound to
    * the type of the handle.
    * Bool tesing is implemented following the safe-bool-idiom.
    * This object is farily lightweight, not much bigger than a pointer type.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    class EntityHandle
    {
    public:
        using typename Data = EntityData<CINDEX, COMP_TOTAL>;
    public:
        /** \brief Returns true, if the entity is still valid, that means the underlying data was not deleted. */
        bool valid() const;

        explicit operator bool() const { return valid(); }

        bool operator==(const EntityHandle<CINDEX, COMP_TOTAL>& other) { return mHandle.block == other.mHandle.block &&
                                                                                mHandle.index == other.mHandle.index &&
                                                                                mGeneration == other.mGeneration; }

        bool operator!=(const EntityHandle<CINDEX, COMP_TOTAL>& other) { return !(*this == other);  }

        /** \brief Returns a pointer to the underlying entity data or a nullptr, if the handle is invalid.
        * Weak referencing is enabled. That means that if the EntityData is deleted through one of its handles,
        * all other handles are aware of that. */
        Data* get();
        Data* get() const;

        /** \brief Enables ptr like remantics. */
        Data* operator->() { return get(); }

        /** \brief Enables ptr like remantics. */
        const Data* operator->() const { return get(); }

        /** \brief Enables ptr like remantics. */
        Data& operator*() { return *get(); }

        /** \brief Enables ptr like remantics. */
        const Data& operator*() const { return *get(); }

    private:
        EntityChunkedArrayHandle mHandle;
        unsigned short mGeneration;
        Universe<CINDEX, COMP_TOTAL>* mUniverse;

    public:
        EntityHandle(Universe<CINDEX, COMP_TOTAL>* cUniverse,
                     EntityChunkedArrayHandle cHandle,
                     unsigned short cGeneration);
    };


    /**
    * \brief A struct that holds meta information for several entities.
    * That information can tell for each component type i what index it
    * has in the componentList of the entity.
    * Note that all entities with the same bitset (that is, the same
    * components assigned) share the same MetaData.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct MetaData
    {
        std::array<CINDEX, COMP_TOTAL> mMetaData;
        unsigned mSharedCount;

        MetaData(std::bitset< COMP_TOTAL > initialMask);
    };


    /**
    * \brief Class that models a specific object in the "world". Entities are simply collections of
    * components. This class can be seen as an interface to manage the components of the
    * entity (add, remove, get, has).
    *
    * The entity class relies on two template arguments. They can be used to control the space (in bits)
    * an entity will require.
    *
    * CIndex - Entities rely on a specific integer type CIndex for their components storage. Note that this
    * type should be always as small as possible since it is the factor that entities grow in size with.
    * The default value is the smallest std integer type unsigned char.
    *
    * CCount - The total number of components of type CIndex. Rising this number will also result in
    * rising entitiy size. The default value is 64.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    class EntityData
    {
    public:
        /**
        * \brief Test if the entity has a specific component of type C. Returns true if and only if the
        * component is assigned. O(1), just a single bit check.
        */
        template<typename C>
        bool has() const;

        /**
        * \brief Adds a new component of type C to the entity, if no other component of that type was added before.
        * Returns success. O(k), where k is the number of assigned components.
        */
        template<typename C, typename ...PARAM>
        void add(PARAM&&... param);

        /**
        * \brief Adds a set of components at once. This is typically more efficient than adding the components one by one.
        * Component types must be default constructible when using this method.
        */
        template<typename ...C>
        void add();

        template<typename ...C>
        void add(MetaData* sampleMeta, std::vector< std::pair<CINDEX, bool> >& vec, std::vector< ComponentHandle >& comphandles);

        /**
        * \brief Gets a const reference to the requested component. Assumes that the component exists.
        * Throws an "No-Component-Found"-error if has<C>() is false when this method is called. O(1).
        */
        template<typename C>
        const C& get() const;

        /**
        * \brief Gets a const reference to the requested component. Assumes that the component exists.
        * Throws an "No-Component-Found"-error if has<C>() is false when this method is called. O(1).
        */
        template<typename C>
        C& modify();

        /**
        * \brief Removes the component of type C from the entity. If the component doesnt exists, the method does nothing.
        */
        template<typename C>
        void rem();

    private:
        std::bitset< COMP_TOTAL > mComponentMask; ///<bit i indicates whether component with ID i is assigned
        MetaData* mMetaData; ///<points to metadata that all entities with the same bitset share
        std::vector< ComponentHandle > mComponentHandles; ///<stores indices of assigned component in their managers
    };



    using ComponentHandle = ElementHandle;

    /**
    * \brief "God-object" for entities and their components.
    * Components of the same type C are stored in semi-continous space.
    * This class provides functionality to create, destroy and retrieve
    * components given a type C and a ComponentHandle.
    * Metadata for entities is also stored semi-continous.
    *
    * Template parameters:
    * CINDEX ... index type for components
    * COMP_TOTAL ... total number of components allowed in the application
    */
    template<typename CINDEX = unsigned char,
             CINDEX COMP_TOTAL = 64>
    class Universe
    {
    public:
        static constexpr std::size_t ENTITY_BLOCK_SIZE = 8192; ///<number of entities in a single, continous memory block
        static constexpr std::size_t COMPONENT_BLOCK_SIZE = 8192; ///<number of components in a single, continous memory block
        static constexpr std::size_t ENTITY_REUSE_C = 1024; ///<minimum stack size until an entity slot is reused

        Universe();

    private:
        /** \brief Checks if the combination of EntityChunkedArrayHandle and generation is still valid. */
        bool valid( EntityChunkedArrayHandle e, unsigned short generation ) const;

        /** \brief Creates an empty entity with a new, unique id and returns a handle to it.*/
        EntityHandle<CINDEX, COMP_TOTAL> create();

        /** \brief Creates an entity with a new, unique id and assigns all given components to it.
        * This is faster then adding the components one by one to an empty entity. */
        template<typename ... C>
        EntityHandle<CINDEX, COMP_TOTAL> create();

        /** \brief Creates n entities with the given components. This is even faster then calling create<C...>()
        * n times and should be the typical way of construction n entities that share the same bitfield. */
        template<typename ... C>
        void create(std::size_t n);

        /** \brief Accesses Entity data through an entity handle. */
        EntityData<CINDEX, COMP_TOTAL>* retrieveEntity( EntityChunkedArrayHandle e, unsigned short generation );

        /** \brief Accesses Entity data through an entity handle. */
        EntityData<CINDEX, COMP_TOTAL>* retrieveEntity( EntityChunkedArrayHandle e, unsigned short generation ) const;

        /** \brief Destroys the given entity and all components assigned to it. */
        void destroy( EntityChunkedArrayHandle e, unsigned short generation );

        /** \brief Assigns a new component and returns a handle to get and destroy that component later. */
        template<typename C, typename ...PARAM>
        ComponentHandle assignComponent(PARAM&&... param);

        /** \brief Retrieves the component of type C and with the given handle.
        * Behavior is undefined if no assignComponent was called before a get call
        * or if the given component handle is invalid. */
        template<typename C>
        C& getComponent(ComponentHandle h);
        template<typename C>
        const C& getComponent(ComponentHandle h) const;

        /** \brief Destroys a component with given typeindex and with the given handle. */
        void destroy(CINDEX cid, ComponentHandle h);

    private:
        std::array< std::unique_ptr<BaseChunkedArray>, COMP_TOTAL> mManagers;
        ChunkedArray<EntityData, ENTITY_BLOCK_SIZE, ENTITY_REUSE_C> mEntityData;
        std::vector<unsigned short> mGenerations; ///<generation counter for each entity id (mapping is COMPONENT_BLOCK_SIZE*block + index)
        std::unordered_map< unsigned long, std::unique_ptr<MetaData> > mComponentMetadata; ///<maps bitset to Metadata
    };


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////IMPLEMENTATION//////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    ChunkedArray<T, BLOCK_SIZE>::ChunkedArray()
    {
        mBlocks.emplace_back();
        mBlocks.back().ptr = alloc.allocate(BLOCK_SIZE);
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    template<typename ...PARAM>
    ChunkedArrayHandle ChunkedArray<T, BLOCK_SIZE>::add(PARAM&&... param)
    {
        ChunkedArrayHandle h;
        if (mFreeSlots.size() > REUSE_C) //reuse a previously abadoned slot
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
        std::allocator_traits<std:allocator>::construct((
                          alloc,
                          mBlocks[h.block].ptr + h.index,
                          std::forward<PARAM>(param)... );  //construct component C in place
        return h;
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    template<typename ...PARAM>
    ChunkedArrayHandle ChunkedArray<T, BLOCK_SIZE>::add(PARAM&&... param, ElementHandle hint)
    {

    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    T& ChunkedArray<T, BLOCK_SIZE>::get(ChunkedArrayHandle h)
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    const T& ChunkedArray<T, BLOCK_SIZE>::get(ChunkedArrayHandle h) const
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    void ChunkedArray<T, BLOCK_SIZE>::destroy(ChunkedArrayHandle h)
    {
        --(mBlocks[h.block].contentCount);
        std::allocator_traits<std:allocator>::destroy(  alloc,
                                                        mBlocks[h.block].ptr + h.index);
        mFreeSlots.push(h);
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    std::size_t ChunkedArray<T, BLOCK_SIZE>::blockCount() const
    {
        return mBlocks.size();
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    ChunkedArray<T, BLOCK_SIZE>::~ChunkedArray()
    {
        for (const auto& block : mBlocks)
        {
            for (std::size_t i = 0; i < BLOCK_SIZE; ++i)
            {
                std::allocator_traits<std:allocator>::destroy(  alloc,
                                                                block.ptr + i);
            }
            alloc.deallocate(block.ptr, BLOCK_SIZE);
        }
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    CINDEX ComponentTraitsBase<CINDEX, COMP_TOTAL>::newID()
    {
        static CINDEX idCounter = 0;
        if (idCounter < COMP_TOTAL)
        {
            return idCounter++;
        }
        else
        {
            throw(ComponentCountError());
        }
    }

    template<typename C, typename CINDEX, CINDEX COMP_TOTAL>
    CINDEX ComponentTraits<C, CINDEX, COMP_TOTAL>::getID()
    {
        const static CINDEX id = ComponentTraitsBase<CINDEX, COMP_TOTAL>::newID();
        return id;
    }

    ComponentCountError::ComponentCountError() :
        std::runtime_error("Attempt to create more than the maximum number of components.") {}




    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL>::EntityHandle(Universe<CINDEX, COMP_TOTAL>* cUniverse,
                                                   EntityChunkedArrayHandle cHandle,
                                                   unsigned short cGeneration)
        : mHandle(cHandle), mGeneration(cGeneration), mUniverse(cUniverse)

    template<typename CINDEX, CINDEX COMP_TOTAL>
    bool EntityHandle<CINDEX, COMP_TOTAL>::valid() const
    {
        return mUniverse->valid( mHandle, mGeneration )
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL>::Data* EntityHandle<CINDEX, COMP_TOTAL>::get()
    {
        return mUniverse->retrieveEntity( mHandle, mGeneration );
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL>::Data* EntityHandle<CINDEX, COMP_TOTAL>::get() const
    {
        return mUniverse->retrieveEntity( mHandle, mGeneration );
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    MetaData<CINDEX, COMP_TOTAL>::MetaData(std::bitset< COMP_TOTAL > initialMask) : mSharedCount(0)
    {
        CINDEX bitc = 0;
        for (CINDEX i = 0; i < COMP_TOTAL; ++i)
        {
            if (initialMask.test(i))
                mMetaData[i] = bitc++;
        }
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    bool Entity<CINDEX, COMP_TOTAL>::has() const
    {
        return mComponentMask.test(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID());
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename ...PARAM>
    void Entity<CINDEX, COMP_TOTAL>::add(PARAM&&... param)
    {
        if (!has<C>()) //dont add if such a component is already assigned
        {
            updateMetaData({{ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), true}});

            ComponentHandle c = mEntityManager.template assignComponent<C, PARAM...>( std::forward<PARAM>(param)... );

            //get the index we have to add the handle in
            auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            mComponentHandles.insert(mComponentHandles.begin() + handleIndex, c);
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ...C>
    void Entity<CINDEX, COMP_TOTAL>::add()
    {
        std::vector< std::pair<CINDEX, bool> > vec;
        std::vector< ComponentHandle > comphandles;
        ComponentUnpacker<CINDEX, COMP_TOTAL, C...>::unpack(vec, comphandles, getEntityManager());
        updateMetaData( vec );
        mComponentHandles.resize(vec.size());
        for (CINDEX i = 0; i < vec.size(); ++i)
        {
            mComponentHandles[ mMetaData->mMetaData[ vec[i].first ] ] = comphandles[i];
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Entity<CINDEX, COMP_TOTAL>::get() const
    {
        auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
        return mEntityManager.template getComponent<C>( mComponentHandles[handleIndex] );
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Entity<CINDEX, COMP_TOTAL>::modify()
    {
        auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
        return mEntityManager.template getComponent<C>( mComponentHandles[handleIndex] );
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    void Entity<CINDEX, COMP_TOTAL>::rem()
    {
        if (has<C>())
        {
            auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            mEntityManager.destroy( ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), mComponentHandles[handleIndex] );
            mComponentHandles.erase( mComponentHandles.begin() + handleIndex );
            updateMetaData({{ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), false}});
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Entity<CINDEX, COMP_TOTAL>::updateMetaData(const std::vector< std::pair<CINDEX, bool> >& val)
    {
        if (mMetaData)
        { //deref to old metadata
            mMetaData->mSharedCount--;
            if (mMetaData->mSharedCount == 0) //no entities with the current bitset anymore, can remove metadata
            {
                auto hashval = mComponentMask.to_ullong();
                getMetaData().erase(hashval);
            }
        }
        for (const auto& i : val)
            mComponentMask.set(i.first, i.second); //set the bits

        auto hashval = mComponentMask.to_ullong(); //get the hash value for the new bitset
        auto meta = getMetaData().emplace( hashval, std::unique_ptr<MetaData>() ); //find metadata, may construct new
        if (meta.second)
            meta.first->second.reset( new MetaData(mComponentMask) );
        mMetaData = meta.first->second.get(); //connect to the metadata
        mMetaData->mSharedCount++;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ...C>
    void Entity<CINDEX, COMP_TOTAL>::add(MetaData* sampleMeta, std::vector< std::pair<CINDEX, bool> >& vec, std::vector< ComponentHandle >& comphandles)
    {
        mMetaData = sampleMeta;
        mMetaData->mSharedCount++;
        mComponentHandles.resize(vec.size());
        for (CINDEX i = 0; i < vec.size(); ++i)
        {
            mComponentHandles[ mMetaData->mMetaData[ vec[i].first ] ] = comphandles[i];
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    std::shared_ptr< Entity<CINDEX, COMP_TOTAL> > Entity<CINDEX, COMP_TOTAL>::create(EntityManager< CINDEX, COMP_TOTAL>& EntityManager)
    {
        struct makeSharedEnabler : public Entity<CINDEX, COMP_TOTAL>
        {
            makeSharedEnabler(EntityManager< CINDEX, COMP_TOTAL>& EntityManager) : Entity<CINDEX, COMP_TOTAL>(EntityManager){}
        };
        return std::make_shared<makeSharedEnabler>(EntityManager);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    Entity<CINDEX, COMP_TOTAL>::~Entity()
    {
        for(CINDEX i = 0; i < COMP_TOTAL; ++i)
        {
            if (mComponentMask.test(i))
            {
                auto handleIndex = mMetaData->mMetaData[ i ];
                mEntityManager.destroy( i, mComponentHandles[handleIndex] );
            }
        }
    }





    template<typename CINDEX, CINDEX COMP_TOTAL>
    bool Universe<CINDEX, COMP_TOTAl>::valid( EntityChunkedArrayHandle e, unsigned short generation ) const
    {
        return mGenerations[ e.block*ENTITY_BLOCK_SIZE + e.index ] == generation;
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL> Universe<CINDEX, COMP_TOTAl>::create()
    {
        EntityChunkedArrayHandle e = mEntityData.add();
        return EntityHandle<CINDEX, COMP_TOTAL>(this, e, mGenerations[ e.block*ENTITY_BLOCK_SIZE + e.index ]);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityData<CINDEX, COMP_TOTAL>* Universe<CINDEX, COMP_TOTAl>::retrieveEntity( EntityChunkedArrayHandle e, unsigned short generation )
    {
        if (valid(e, generation))  return *mEntityData.get(e);
        else  return nullptr;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityData<CINDEX, COMP_TOTAL>* Universe<CINDEX, COMP_TOTAl>::retrieveEntity( EntityChunkedArrayHandle e, unsigned short generation ) const
    {
        if (valid(e, generation))  return *mEntityData.get(e);
        else  return nullptr;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename ...PARAM>
    ComponentHandle Universe<CINDEX, COMP_TOTAl>::assignComponent(PARAM&&... param)
    {
        //create the corresponding container if not existing yet
        if (!mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ])
        {
            mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ] =
                std::unique_ptr<BaseChunkedArray>( new ChunkedArray<C, COMPONTENT_BLOCK_SIZE>() );
        }
        ChunkedArray<C, COMPONTENT_BLOCK_SIZE>* ca = static_cast<ChunkedArray<C, COMPONTENT_BLOCK_SIZE>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->add( std::forward<PARAM>(param)... );
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAl>::destroy( EntityChunkedArrayHandle e, unsigned short generation )
    {
        if (!valid(e, generation))
            return;
        mEntityData.destroy(e); //destructor of EntityData takes care of destroying the components
        mGenerations[ e.block*ENTITY_BLOCK_SIZE + e.index ] ++; //invalidates all handles pointing to the deleted entity
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Universe<CINDEX, COMP_TOTAl>::getComponent(ComponentHandle h)
    {
        ChunkedArray<C, COMPONTENT_BLOCK_SIZE>* ca = static_cast<ChunkedArray<C, COMPONTENT_BLOCK_SIZE>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->get(h);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Universe<CINDEX, COMP_TOTAl>::getComponent(ComponentHandle h) const
    {
        ChunkedArray<C, COMPONTENT_BLOCK_SIZE>* ca = static_cast<ChunkedArray<C, COMPONTENT_BLOCK_SIZE>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->get(h);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAl>::destroy(CINDEX cid, ComponentHandle h)
    {
        mManagers[ cid ].get()->destroy(h);
    }
}

#endif // DOM_LIBRARY_H
