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

#ifndef DOM_ENTITY_H
#define DOM_ENTITY_H

#include <bitset>
#include <array>
#include <unordered_map>

#include "ComponentUnpacker.h"
#include "EntityManager.h"

namespace dom
{
    template <typename CINDEX, CINDEX COMP_TOTAL> class Universe;

    using EntityHandle = ElementHandle;

    /**
    * \brief A struct that holds meta information for several entities.
    * That information can tell for each component type i what index it
    * has in the componentList of the entity.
    * Note that all entities with the same bitset (that is, the same
    * components assigned) share the same MetaData.
    */
    struct MetaData
    {
        std::array<CINDEX, COMP_TOTAL> mMetaData;
        unsigned mSharedCount;

        MetaData(std::bitset< COMP_TOTAL > initialMask);
    };

    /**
    * \brief A struct that holds information that is unique to a single entity.
    * This information can be accessed through an entity instance, if the handle
    * is still valid.
    * The entity data will be stored semi continous just like components.
    */
    struct EntityData
    {
        std::bitset< COMP_TOTAL > mComponentMask; ///<bit i indicates whether component with ID i is assigned
        MetaData* mMetaData; ///<points to metadata that all entities with the same bitset share
        std::vector< ComponentHandle > mComponentHandles; ///<stores indices of assigned component in their managers
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
    class Entity
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

        /**
        * \brief Creates a new entity wrapped in a shared ptr.
        */
        static std::shared_ptr< Entity<CINDEX, COMP_TOTAL> > create(EntityManager< CINDEX, COMP_TOTAL>& EntityManager);

        /** \brief Returns a reference to the EntityManager of that entity. */
        EntityManager< CINDEX, COMP_TOTAL>& getEntityManager() { return mEntityManager; }

    public:
        Entity(EntityManager< CINDEX, COMP_TOTAL>& EntityManager) : mMetaData(nullptr), mEntityManager(EntityManager) {}

    private:
        EntityHandle mHandle;
        EntityManager< CINDEX, COMP_TOTAL>& mEntityManager;

        void updateMetaData(const std::vector< std::pair<CINDEX, bool> >& val);

    public:
        //forbit copying of an entity
        Entity(const Entity& e) = delete;
        Entity & operator=(const Entity&) = delete;

        static std::unordered_map< unsigned long, std::unique_ptr<MetaData> >& getMetaData()
        {
            static std::unordered_map< unsigned long, std::unique_ptr<MetaData> > sEntitiyData;
            return sEntitiyData;
        }

        ~Entity();
    };

    ///IMPLEMENTATION

    template<typename CINDEX, CINDEX COMP_TOTAL>
    Entity<CINDEX, COMP_TOTAL>::MetaData::MetaData(std::bitset< COMP_TOTAL > initialMask) : mSharedCount(0)
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
}

#endif // DOM_ENTITY_H
