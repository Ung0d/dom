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
#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <bitset>
#include <array>
#include <unordered_map>
#include <stdexcept>
#include <tuple>
#include <limits>

namespace dom
{
    template <typename CINDEX, CINDEX COMP_TOTAL> class Universe;

    /** \brief A handle object to access an element in the ChunkedArray.
    * Works with double indexing (block,index). Occupies 32 bit. */
    struct ChunkedArrayHandle
    {
        unsigned short block;
        unsigned short index;

        ChunkedArrayHandle() {}
        ChunkedArrayHandle(unsigned short b, unsigned short i) : block(b), index(i) {}

        /** \brief Returns a handle that compares false to every other handle. */
        static ChunkedArrayHandle null()
        {
            static ChunkedArrayHandle nullHandle(std::numeric_limits<unsigned short>::max(), std::numeric_limits<unsigned short>::max());
            return nullHandle;
        }

        /** \brief Implements comparison. */
        bool operator==(const ChunkedArrayHandle& other) const { return block == other.block && index == other.index; }

        /** \brief Implements comparison. */
        bool operator!=(const ChunkedArrayHandle& other) const { return !(*this == other);  }

        /** \brief Implements bool checking. */
        explicit operator bool() const { return *this != null(); }
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
    * Note that destruction of the objects in the ChunkedArray cant be done by the
    * array itself. The user must take care of destroying all created objects itself
    * by call destroy(handle). The reason behind this is, that the array does not hold an
    * list of the slots with constructed object in itself. But the user of the array should
    * have.
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
        friend class ChunkedArray<T, BLOCK_SIZE, REUSE_C>;
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



    using EntityArrayHandle = ChunkedArrayHandle;
    using ComponentHandle = ChunkedArrayHandle;

    template<typename CINDEX, CINDEX COMP_TOTAL> class EntityData;
    template<typename CINDEX, CINDEX COMP_TOTAL> class Universe;
    template<typename C, typename ENABLE> class ComponentInstantiator;


    /**
    * \brief An interfaces class for components that can be attached multiple times to the entity.
    * The component set is implemented in single linked list manner. This interface will add a
    * small overhead (a pointer to next) to the component.
    * When using the dom::iterate functionality with components C, that derive from dom::MultiComponent<C>,
    * the given callback is invoked for each component of type C that is attached to the entity.
    * Note that MultiComponent-sets must not persist continously in memory, but if they are
    * instantiated at once, the chances are good that they are.
    */
    template<typename C>
    class MultiComponent
    {
    public:
        ComponentHandle next;

        MultiComponent();

        template<typename CINDEX, CINDEX COMP_TOTAL>
        C* getNext(Universe<CINDEX, COMP_TOTAL>& universe);
        template<typename CINDEX, CINDEX COMP_TOTAL>
        const C* getNext(Universe<CINDEX, COMP_TOTAL>& universe) const;
    };

    /** \brief A Utility struct used to construct a component with parameters. */
    template<typename C, typename ENABLE = void>
    struct ComponentInstantiator
    {
    public:
        using COMP_TYPE = C;

        template<typename CINDEX, CINDEX COMP_TOTAL, typename ... PARAM>
        ComponentInstantiator(Universe<CINDEX, COMP_TOTAL>& universe, PARAM&& ... param);


        template<typename CINDEX, CINDEX COMP_TOTAL>
        void clear(Universe<CINDEX, COMP_TOTAL>& universe);

        ComponentHandle handle;
    };

    template<typename C>
    struct ComponentInstantiator<C, typename std::enable_if<std::is_base_of<MultiComponent<C>, C>::value>::type>
    {
    public:
        using COMP_TYPE = C;

        template<typename CINDEX, CINDEX COMP_TOTAL, typename ... PARAM>
        ComponentInstantiator(Universe<CINDEX, COMP_TOTAL>& universe, std::size_t num, PARAM&& ... param);

        template<typename CINDEX, CINDEX COMP_TOTAL>
        void clear(Universe<CINDEX, COMP_TOTAL>& universe);

        ComponentHandle handle;
    };


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
    friend class Universe<CINDEX, COMP_TOTAL>;
    public:
        using Data = EntityData<CINDEX, COMP_TOTAL>;

    public:
        /** \brief Returns true, if the entity is still valid, that means the underlying data was not deleted. */
        bool valid() const;

        explicit operator bool() const { return valid(); }

        bool operator==(const EntityHandle<CINDEX, COMP_TOTAL>& other) const { return mHandle == other.mHandle &&
                                                                                mGeneration == other.mGeneration; }

        bool operator!=(const EntityHandle<CINDEX, COMP_TOTAL>& other) const { return !(*this == other);  }


        /**
        * \brief Test if the entity has a specific component of type C. Returns true if and only if the
        * component is assigned. O(1), just a single bit check.
        */
        template<typename C>
        bool has() const;

        /**
        * \brief Destroys the underlying entity through the handle. This will automatically invalidate all other handles.
        */
        void destroy() const;

        /**
        * \brief Adds a new component of type C to the entity, if no other component of that type was added before.
        * Returns success. O(k), where k is the number of omponents of fthe entity.
        * Adds a set of components at once. This is typically more efficient than adding the components one by one.
        * Component types must be default constructible when using this method.
        */
        template<typename ... C>
        void add() const;

        /** \brief same as add, but uses dom::instantiate to construct components with parameters. */
        template<typename ... C>
        void add(ComponentInstantiator<C>... ci) const;

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
        C& modify() const;

        /**
        * \brief Removes the component of type C from the entity. If the component doesnt exists, the method does nothing.
        */
        template<typename C>
        void rem() const;


        /** \brief Returns the unique id of the entity. */
        std::size_t getID() const;

        /** \brief Returns a handle that will never be valid. Equivalent to what nullptr is for normal pointers. */
        static EntityHandle nullEntity();

    private:
        EntityArrayHandle mHandle;
        unsigned short mGeneration;
        Universe<CINDEX, COMP_TOTAL> * mUniverse;

    private:
        EntityHandle(Universe<CINDEX, COMP_TOTAL>* cUniverse,
                     EntityArrayHandle cHandle,
                     unsigned short cGeneration);
    };

    /**
    * \brief A struct that holds meta information for several entities.
    * All entities with the same components assigned share the same metadata.
    * Delivers information that can tell for each component type i what index it
    * has in the componentList of the entity.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct MetaData
    {
    friend class EntityData<CINDEX, COMP_TOTAL>;
    friend class Universe<CINDEX, COMP_TOTAL>;
    private:
        std::bitset< COMP_TOTAL > mComponentMask;
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
    friend class Universe<CINDEX, COMP_TOTAL>;
    private:
        MetaData<CINDEX, COMP_TOTAL>* mMetaData; ///<points to metadata that all entities with the same bitset share
        std::vector< ComponentHandle > mComponentHandles; ///<stores indices of assigned component in their managers
    };


    /**
    * \brief Factory- and master-object for entities and their components.
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
    friend class EntityHandle<CINDEX, COMP_TOTAL>;
    template <class C> friend class MultiComponent;
    template <class C, class ENABLE> friend class ComponentInstantiator;
    public:
        static constexpr std::size_t ENTITY_BLOCK_SIZE = 8192; ///<number of entities in a single, continous memory block
        static constexpr std::size_t COMPONENT_BLOCK_SIZE = 8192; ///<number of components in a single, continous memory block
        static constexpr std::size_t ENTITY_REUSE_C = 1024; ///<minimum stack size until an entity slot is reused

        Universe();

    public:
        /** \brief Creates an empty entity with a new, unique id and returns a handle to it.*/
        EntityHandle<CINDEX, COMP_TOTAL> create();

        /** \brief Creates an entity with a new, unique id and assigns all given components to it.
        * This is faster then adding the components one by one to an empty entity. */
        template<typename ... C>
        EntityHandle<CINDEX, COMP_TOTAL> create();
        template<typename ... C>
        EntityHandle<CINDEX, COMP_TOTAL> create(ComponentInstantiator<C>... ci);

        /** \brief Creates n entities with the given components. This is even faster then calling create<C...>()
        * n times and should be the typical way of construction n entities that share the same bitfield.
        * The function f is called for each created entity. */
        template<typename ... C>
        void create(std::size_t n, std::function<void(EntityHandle<CINDEX, COMP_TOTAL> e)> f);

    private:
        /** \brief Checks if entityData belonging to the given handle is still valid. */
        bool valid( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const;

        /** \brief Destroys the given entity and all components assigned to it. */
        void destroyEntity( const EntityHandle<CINDEX, COMP_TOTAL>& e );

        /**
        * \brief Test if the entity has a specific component of type C. Returns true if and only if the
        * component is assigned. O(1), just a single bit check.
        */
        template<typename C>
        bool hasComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const;

        /**
        * \brief Adds a new component of type C to the entity, if no other component of that type was added before.
        * Returns success. O(k), where k is the number of assigned components.
        * Adds a set of components at once. This is typically more efficient than adding the components one by one.
        */
        template<typename ... C>
        void addComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e, ComponentInstantiator<C>... ci );

        /**
        * \brief Gets a const reference to the requested component. Assumes that the component exists.
        * Throws an "No-Component-Found"-error if has<C>() is false when this method is called. O(1).
        */
        template<typename C>
        const C& getComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const;
        template<typename C>
        const C& getComponent( ComponentHandle c ) const;

        /**
        * \brief Gets a const reference to the requested component. Assumes that the component exists.
        * Throws an "No-Component-Found"-error if has<C>() is false when this method is called. O(1).
        */
        template<typename C>
        C& modifyComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e );
        template<typename C>
        C& modifyComponent( ComponentHandle c );

        /**
        * \brief Removes the component of type C from the entity. If the component doesnt exists, the method does nothing.
        */
        template<typename C, typename std::enable_if<!std::is_base_of<MultiComponent<C>, C>::value>::type* = nullptr>
        void removeComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e );
        template<typename C, typename std::enable_if<std::is_base_of<MultiComponent<C>, C>::value>::type* = nullptr>
        void removeComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e );

        /** \brief Computes and returns the id of the given entity. */
        std::size_t getID( const EntityArrayHandle& e );

        /** \brief After calling this method for an entity, its ensured that the internal
        * datastructes are capable of the (new) entity. */
        void accommodateEntity( const EntityArrayHandle& e );

        /** \brief Called when connected to an EntityData. */
        void connect(EntityData<CINDEX, COMP_TOTAL>& data, std::bitset<COMP_TOTAL> mask);

        /** \brief Called when disconnected from an EntityData. */
        void disconnect(const EntityData<CINDEX, COMP_TOTAL>& data);

    public:
        /** \brief Helper method to instantiate components with parameters. */
        template<typename C, typename ... PARAM>
        ComponentInstantiator<C> instantiate(PARAM&&... param);

    private:
        std::array< std::unique_ptr<BaseChunkedArray>, COMP_TOTAL> mManagers;
        ChunkedArray<EntityData<CINDEX, COMP_TOTAL>, ENTITY_BLOCK_SIZE, ENTITY_REUSE_C> mEntityData;
        std::vector<unsigned short> mGenerations; ///<generation counter for each entity id (mapping is COMPONENT_BLOCK_SIZE*block + index)
        std::unordered_map< unsigned long, std::unique_ptr<MetaData<CINDEX, COMP_TOTAL>> > mComponentMetadata; ///<maps bitset to Metadata
        MetaData<CINDEX, COMP_TOTAL> mEmptyMeta;

        template <typename... C>
        struct ComponentUnpacker;
    };


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////TEMPLATE_UNPACKING//////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename CINDEX, CINDEX COMP_TOTAL>
    template < typename C1, typename... C>
    struct Universe<CINDEX, COMP_TOTAL>::ComponentUnpacker<C1, C...>
    {
        static void unpack(Universe<CINDEX, COMP_TOTAL>& universe,
                           std::vector< ComponentHandle >& handles,
                           std::bitset< COMP_TOTAL >& mask,
                           ComponentInstantiator<C1> c1,
                           ComponentInstantiator<C>... c)
        {
            if (!mask.test(ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID())) //dont add if such a component is already assigned
            {
                //store the handle
                handles.emplace_back(c1.handle);
                mask.set(ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID());
            }
            else //clear the created component if the user made a mistake and added it twice
            {
                c1.clear(universe);
            }
            //recursive...
            ComponentUnpacker<C...>::unpack(universe, handles, mask, c...);
        }

        static void unpack(std::vector< ComponentHandle >& handles,
                           ComponentInstantiator<C1> c1,
                           ComponentInstantiator<C>... c)
        {
            //store the handle
            handles.emplace_back(c1.handle);
            //recursive...
            ComponentUnpacker<C...>::unpack(handles, c...);
        }

        static void sort(std::vector< ComponentHandle >& handles,
                         std::array< CINDEX, COMP_TOTAL >& positions,
                         CINDEX i = 0)
        {
            if (i != positions[ ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID() ])
            {
                std::swap(handles[i], handles[ ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID() ]);
            }
            ComponentUnpacker<C...>::sort(handles, positions, ++i);
        }
    };

    template <typename CINDEX, CINDEX COMP_TOTAL>
    template <typename C1>
    struct Universe<CINDEX, COMP_TOTAL>::ComponentUnpacker<C1>
    {
        static void unpack(Universe<CINDEX, COMP_TOTAL>& universe,
                           std::vector< ComponentHandle >& handles,
                           std::bitset< COMP_TOTAL >& mask,
                           ComponentInstantiator<C1> c1)
        {
            if (!mask.test(ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID())) //dont add if such a component is already assigned
            {
                handles.emplace_back(c1.handle);
                mask.set(ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID());
            }
            else //clear the created component if the user made a mistake and added it twice
            {
                c1.clear(universe);
            }
        }

        static void unpack(std::vector< ComponentHandle >& handles,
                           ComponentInstantiator<C1> c1)
        {
            handles.emplace_back(c1.handle);
        }

        static void sort(std::vector< ComponentHandle >& handles,
                         std::array< CINDEX, COMP_TOTAL >& positions,
                         CINDEX i)
        {
            if (i != positions[ ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID() ])
            {
                std::swap(handles[i], handles[ ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID() ]);
            }
        }
    };


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////IMPLEMENTATION//////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    ChunkedArray<T, BLOCK_SIZE, REUSE_C>::ChunkedArray()
    {
        mBlocks.emplace_back();
        mBlocks.back().ptr = alloc.allocate(BLOCK_SIZE);
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    template<typename ...PARAM>
    ChunkedArrayHandle ChunkedArray<T, BLOCK_SIZE, REUSE_C>::add(PARAM&&... param)
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
        std::allocator_traits<std::allocator<T>>::construct(
                          alloc,
                          mBlocks[h.block].ptr + h.index,
                          std::forward<PARAM>(param)... );  //construct component C in place
        return h;
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    T& ChunkedArray<T, BLOCK_SIZE, REUSE_C>::get(ChunkedArrayHandle h)
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    const T& ChunkedArray<T, BLOCK_SIZE, REUSE_C>::get(ChunkedArrayHandle h) const
    {
        return (mBlocks[h.block].ptr)[h.index];
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    void ChunkedArray<T, BLOCK_SIZE, REUSE_C>::destroy(ChunkedArrayHandle h)
    {
        --(mBlocks[h.block].contentCount);
        std::allocator_traits<std::allocator<T>>::destroy(  alloc,
                                                        mBlocks[h.block].ptr + h.index);
        mFreeSlots.push(h);
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    std::size_t ChunkedArray<T, BLOCK_SIZE, REUSE_C>::blockCount() const
    {
        return mBlocks.size();
    }

    template<typename T, std::size_t BLOCK_SIZE, std::size_t REUSE_C>
    ChunkedArray<T, BLOCK_SIZE, REUSE_C>::~ChunkedArray()
    {
        for (const auto& block : mBlocks)
        {
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

    inline ComponentCountError::ComponentCountError() :
        std::runtime_error("Attempt to create more than the maximum number of components.") {}




    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL>::EntityHandle(Universe<CINDEX, COMP_TOTAL>* cUniverse,
                                                   EntityArrayHandle cHandle,
                                                   unsigned short cGeneration)
        : mHandle(cHandle), mGeneration(cGeneration), mUniverse(cUniverse) {}

    template<typename CINDEX, CINDEX COMP_TOTAL>
    bool EntityHandle<CINDEX, COMP_TOTAL>::valid() const
    {
        return mUniverse->valid(*this);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void EntityHandle<CINDEX, COMP_TOTAL>::destroy() const
    {
        return mUniverse->destroyEntity(*this);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    bool EntityHandle<CINDEX, COMP_TOTAL>::has() const
    {
        return mUniverse->template hasComponent<C>(*this);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    void EntityHandle<CINDEX, COMP_TOTAL>::add() const
    {
        mUniverse->template addComponent<C...>(*this, ComponentInstantiator<C>(*mUniverse)...);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    void EntityHandle<CINDEX, COMP_TOTAL>::add(ComponentInstantiator<C>... ci) const
    {
        mUniverse->template addComponent<C...>(*this, ci...);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& EntityHandle<CINDEX, COMP_TOTAL>::get() const
    {
        return mUniverse->template getComponent<C>(*this);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& EntityHandle<CINDEX, COMP_TOTAL>::modify() const
    {
        return mUniverse->template modifyComponent<C>(*this);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    void EntityHandle<CINDEX, COMP_TOTAL>::rem() const
    {
        mUniverse->template removeComponent<C>(*this);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    std::size_t EntityHandle<CINDEX, COMP_TOTAL>::getID() const
    {
        return mUniverse->getID(mHandle);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL> EntityHandle<CINDEX, COMP_TOTAL>::nullEntity()
    {
        return EntityHandle<CINDEX, COMP_TOTAL>(nullptr, {0,0}, 0);
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    MetaData<CINDEX, COMP_TOTAL>::MetaData(std::bitset< COMP_TOTAL > initialMask) : mComponentMask(initialMask), mSharedCount(0)
    {
        CINDEX bitc = 0;
        for (CINDEX i = 0; i < COMP_TOTAL; ++i)
        {
            if (initialMask.test(i))
                mMetaData[i] = bitc++;
        }
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    Universe<CINDEX, COMP_TOTAL>::Universe() : mEmptyMeta(std::bitset<COMP_TOTAL>()) {}

    template<typename CINDEX, CINDEX COMP_TOTAL>
    bool Universe<CINDEX, COMP_TOTAL>::valid( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const
    {
        return mGenerations[ e.mHandle.block*ENTITY_BLOCK_SIZE + e.mHandle.index ] == e.mGeneration;
    }



    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityHandle<CINDEX, COMP_TOTAL> Universe<CINDEX, COMP_TOTAL>::create()
    {
        EntityArrayHandle e = mEntityData.add();
        mEntityData.get(e).mMetaData = &mEmptyMeta;
        accommodateEntity(e);
        return EntityHandle<CINDEX, COMP_TOTAL>(this, e, mGenerations[ e.block*ENTITY_BLOCK_SIZE + e.index ]);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    EntityHandle<CINDEX, COMP_TOTAL> Universe<CINDEX, COMP_TOTAL>::create()
    {
        return create<C...>(ComponentInstantiator<C>(*this)...);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    EntityHandle<CINDEX, COMP_TOTAL> Universe<CINDEX, COMP_TOTAL>::create(ComponentInstantiator<C>... ci)
    {
        auto e = create(); //create empty entity
        e.template add<C...>(ci...);
        return e;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    void Universe<CINDEX, COMP_TOTAL>::create(std::size_t n,
                                              std::function<void(EntityHandle<CINDEX, COMP_TOTAL> e)> f)
    {
        if (n > 0)
        {
            std::size_t num = sizeof...(C);
            MetaData<CINDEX, COMP_TOTAL>* sampleMeta;
            EntityArrayHandle ehandle = mEntityData.add();
            accommodateEntity(ehandle);
            EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(ehandle);
            data.mComponentHandles.reserve(num);
            std::bitset<COMP_TOTAL> sampleMask;
            ComponentUnpacker<C...>::unpack(*this, data.mComponentHandles, sampleMask, ComponentInstantiator<C>(*this)...);
            connect(data, sampleMask);
            sampleMeta = data.mMetaData;
            ComponentUnpacker<C...>::sort(data.mComponentHandles, data.mMetaData->mMetaData, 0);
            f(EntityHandle<CINDEX, COMP_TOTAL>(this, ehandle, mGenerations[ ehandle.block*ENTITY_BLOCK_SIZE + ehandle.index ]));

            for(std::size_t i = 0; i < n; ++i)
            {
                EntityArrayHandle ehandle = mEntityData.add();
                accommodateEntity(ehandle);
                EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(ehandle);
                data.mComponentHandles.reserve(num);
                ComponentUnpacker<C...>::unpack(data.mComponentHandles, ComponentInstantiator<C>(*this)...);
                data.mMetaData = sampleMeta;
                sampleMeta->mSharedCount++;
                ComponentUnpacker<C...>::sort(data.mComponentHandles, data.mMetaData->mMetaData, 0);
                f(EntityHandle<CINDEX, COMP_TOTAL>(this, ehandle, mGenerations[ ehandle.block*ENTITY_BLOCK_SIZE + ehandle.index ]));
            }
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAL>::destroyEntity( const EntityHandle<CINDEX, COMP_TOTAL>& e )
    {
        if (!valid(e)) return;
        EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(e.mHandle);
        for(CINDEX i = 0; i < COMP_TOTAL; ++i) //remove all components
        {
            if (data.mMetaData->mComponentMask.test(i))
            {
                auto handleIndex = data.mComponentHandles[data.mMetaData->mMetaData[ i ]];
                mManagers[ i ].get()->destroy(handleIndex);
            }
        }
        mEntityData.destroy(e.mHandle);
        mGenerations[ e.mHandle.block*ENTITY_BLOCK_SIZE + e.mHandle.index ] ++; //invalidates all handles pointing to the deleted entity
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    bool Universe<CINDEX, COMP_TOTAL>::hasComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const
    {
        return mEntityData.get(e.mHandle).mMetaData->mComponentMask.test(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID());
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename ... C>
    void Universe<CINDEX, COMP_TOTAL>::addComponent(const EntityHandle<CINDEX, COMP_TOTAL>& e, ComponentInstantiator<C>... ci)
    {
        EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(e.mHandle);
        CINDEX sizeBefore = data.mComponentHandles.size();
        std::bitset<COMP_TOTAL> mask = data.mMetaData->mComponentMask;
        ComponentUnpacker<C...>::unpack(*this, data.mComponentHandles, mask, ci...);
        disconnect(data);
        connect(data, mask);
        ComponentUnpacker<C...>::sort(data.mComponentHandles, data.mMetaData->mMetaData,sizeBefore);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Universe<CINDEX, COMP_TOTAL>::modifyComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e )
    {
        return const_cast<C&>( static_cast<const Universe<CINDEX, COMP_TOTAL>*>(this)->getComponent<C>(e) );
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Universe<CINDEX, COMP_TOTAL>::modifyComponent( ComponentHandle c )
    {
        return const_cast<C&>( static_cast<const Universe<CINDEX, COMP_TOTAL>*>(this)->getComponent<C>(c) );
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Universe<CINDEX, COMP_TOTAL>::getComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e ) const
    {
        const EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(e.mHandle);
        auto handleIndex = data.mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
        return getComponent<C>(data.mComponentHandles[handleIndex]);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Universe<CINDEX, COMP_TOTAL>::getComponent( ComponentHandle c ) const
    {
        ChunkedArray<C, COMPONENT_BLOCK_SIZE>* ca = static_cast<ChunkedArray<C, COMPONENT_BLOCK_SIZE>*>
                                                        ( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->get(c);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename std::enable_if<!std::is_base_of<MultiComponent<C>, C>::value>::type*>
    void Universe<CINDEX, COMP_TOTAL>::removeComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e )
    {
        if (hasComponent<C>(e))
        {
            EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(e.mHandle);
            auto handleIndex = data.mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get()->destroy( data.mComponentHandles[handleIndex] );
            data.mComponentHandles.erase( data.mComponentHandles.begin() + handleIndex );

            std::bitset<COMP_TOTAL> mask = data.mMetaData->mComponentMask;
            mask.set(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), false); //set the bit
            disconnect(data);
            connect(data, mask);
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename std::enable_if<std::is_base_of<MultiComponent<C>, C>::value>::type*>
    void Universe<CINDEX, COMP_TOTAL>::removeComponent( const EntityHandle<CINDEX, COMP_TOTAL>& e )
    {
        if (hasComponent<C>(e))
        {
            EntityData<CINDEX, COMP_TOTAL>& data = mEntityData.get(e.mHandle);
            auto handleIndex = data.mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            ComponentHandle current = data.mComponentHandles[handleIndex];
            ComponentHandle next = ComponentHandle::null();
            while (current)
            {
                next = mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get()->get(current).next;
                mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get()->destroy(current);
                current = next;
            }
            data.mComponentHandles.erase( data.mComponentHandles.begin() + handleIndex );

            std::bitset<COMP_TOTAL> mask = data.mMetaData->mComponentMask;
            mask.set(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), false); //set the bit
            disconnect(data);
            connect(data, mask);
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    std::size_t Universe<CINDEX, COMP_TOTAL>::getID( const EntityArrayHandle& e )
    {
        return e.block*ENTITY_BLOCK_SIZE + e.index;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAL>::accommodateEntity( const EntityArrayHandle& e )
    {
        std::size_t id = getID(e);
        if (mGenerations.size() <= id)
            mGenerations.resize(id+1, 1);
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAL>::connect(EntityData<CINDEX, COMP_TOTAL>& data, std::bitset<COMP_TOTAL> mask)
    {
        auto hashval = mask.to_ullong(); //get the hash value for the current bitset
        auto meta = mComponentMetadata.emplace( hashval, std::unique_ptr<MetaData<CINDEX, COMP_TOTAL>>() ); //find metadata, may construct new
        if (meta.second)
            meta.first->second.reset( new MetaData<CINDEX, COMP_TOTAL>(mask) );
        data.mMetaData = meta.first->second.get(); //connect to the metadata
        data.mMetaData->mSharedCount++;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Universe<CINDEX, COMP_TOTAL>::disconnect(const EntityData<CINDEX, COMP_TOTAL>& data)
    {
        data.mMetaData->mSharedCount--;
        if (data.mMetaData->mSharedCount == 0) //no entities with the current bitset anymore, can remove metadata
        {
            auto hashval = data.mMetaData->mComponentMask.to_ullong();
            mComponentMetadata.erase(hashval);
        }
    }


    template<typename C>
    MultiComponent<C>::MultiComponent() : next(ComponentHandle::null()) {}

    template<typename C>
    template<typename CINDEX, CINDEX COMP_TOTAL>
    C* MultiComponent<C>::getNext(Universe<CINDEX, COMP_TOTAL>& universe)
    {
        if (next)
            return &universe.template modifyComponent<C>(next);
        else return nullptr;
    }

    template<typename C>
    template<typename CINDEX, CINDEX COMP_TOTAL>
    const C* MultiComponent<C>::getNext(Universe<CINDEX, COMP_TOTAL>& universe) const
    {
        if (next)
            return &universe.template getComponent<C>(next);
        else return nullptr;
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename ... PARAM>
    ComponentInstantiator<C> Universe<CINDEX, COMP_TOTAL>::instantiate(PARAM&&... param)
    {
        return ComponentInstantiator<C>( *this, std::forward<PARAM>(param)... );
    }



    template<typename C, typename ENABLE>
    template<typename CINDEX, CINDEX COMP_TOTAL, typename ... PARAM>
    ComponentInstantiator<C, ENABLE>::ComponentInstantiator(Universe<CINDEX, COMP_TOTAL>& universe, PARAM&& ... param)
    {
        //create the corresponding container if not existing yet
        if (!universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ])
        {
            universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ] =
                std::unique_ptr<BaseChunkedArray>( new ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>() );
        }
        ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>* ca =
                static_cast<ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>*>( universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        handle = ca->add( std::forward<PARAM>(param)... );
    }


    template<typename C, typename ENABLE>
    template<typename CINDEX, CINDEX COMP_TOTAL>
    void ComponentInstantiator<C, ENABLE>::clear(Universe<CINDEX, COMP_TOTAL>& universe)
    {
        ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>* ca =
                static_cast<ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>*>( universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        ca->destroy(handle);
    }


    template<typename C>
    template<typename CINDEX, CINDEX COMP_TOTAL, typename ... PARAM>
    ComponentInstantiator<C, typename std::enable_if<std::is_base_of<MultiComponent<C>, C>::value>::type>::
        ComponentInstantiator(Universe<CINDEX, COMP_TOTAL>& universe, std::size_t num, PARAM&& ... param)
    {
        //create the corresponding container if not existing yet
        if (!universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ])
        {
            universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ] =
                std::unique_ptr<BaseChunkedArray>( new ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>() );
        }
        ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>* ca =
                static_cast<ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>*>( universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        handle = ca->add( std::forward<PARAM>(param)... );
        ComponentHandle current = handle;
        for (std::size_t i = 0; i < num; ++i)
        { //build the single linked list
            C& c = ca->get(current);
            c.next = ca->add( std::forward<PARAM>(param)... );
            current = c.next;
        }
    }


    template<typename C>
    template<typename CINDEX, CINDEX COMP_TOTAL>
    void ComponentInstantiator<C, typename std::enable_if<std::is_base_of<MultiComponent<C>, C>::value>::type>::
        clear(Universe<CINDEX, COMP_TOTAL>& universe)
    {
        ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>* ca =
                static_cast<ChunkedArray<C, universe.COMPONENT_BLOCK_SIZE>*>( universe.mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        ComponentHandle current = handle;
        ComponentHandle next = ComponentHandle::null();
        while (current)
        {
            next = ca->get(current).next;
            ca->destroy(current);
            current = next;
        }
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////UTILITY_METHODS/////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    template< typename EN, typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct Utility
    {
        template <typename T> struct Identity { typedef T type; };

        template <typename... C>
        struct ComponentChecker;

        template <typename ... C>
        static void iterate( const std::list<EN>& entities,
                      typename Identity<std::function<void(EN, C&...)>>::type f);
    };


    template< typename EN, typename CINDEX, CINDEX COMP_TOTAL>
    template <typename ... C>
    void Utility<EN, CINDEX, COMP_TOTAL>::iterate(
                         const std::list<EN>& entities,
                         typename Identity<std::function<void(EN, C&...)>>::type f)
    {
        for (auto& e : entities)
        {
            if (ComponentChecker<C...>::check(e))
            {
                f(e, e.template modify<C>()... );
            }
        }
    }

    template< typename EN, typename CINDEX, CINDEX COMP_TOTAL>
    template <typename C1, typename... C>
    struct Utility<EN, CINDEX, COMP_TOTAL>::ComponentChecker<C1, C...>
    {
      static bool check(const EN& e)
      {
          return e.template has<C1>() && Utility<EN, CINDEX, COMP_TOTAL>::ComponentChecker<C...>::check(e);
      }
    };

    template< typename EN, typename CINDEX, CINDEX COMP_TOTAL>
    template <typename C1>
    struct Utility<EN, CINDEX, COMP_TOTAL>::ComponentChecker<C1>
    {
      static bool check(const EN& e)
      {
        return e.template has<C1>();
      }
    };
}

#endif // DOM_LIBRARY_H
