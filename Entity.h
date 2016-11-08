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

#include "ComponentTraits.h"
#include "ComponentManager.h"

namespace dom
{
    /**
    * \brief A struct that holds meta information for several entities.
    * That information can tell for each component type i what index it
    * has in the componentList of the entity.
    * Note that all entities with the same bitset (that is, the same
    * components assigned) share the same entityData.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    class EntityData
    {
    friend class Entity<CINDEX, COMP_TOTAL>;
    private:
        std::array<CINDEX, COMP_TOTAL> mMetaData;
        unsigned mSharedCount;

    public:
        EntityData(std::bitset< COMP_TOTAL > initialMask);
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
        bool add(PARAM&&... param);

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
        static std::shared_ptr< Entity<CINDEX, COMP_TOTAL> > create(ComponentManager< CINDEX, COMP_TOTAL>& componentManager);

    private:
        Entity(ComponentManager< CINDEX, COMP_TOTAL>& componentManager) : mMetaData(nullptr), mComponentManager(componentManager) {}

    private:
        std::bitset< COMP_TOTAL > mComponentMask; ///<bit i indicates whether component with ID i is assigned
        EntityData<CINDEX, COMP_TOTAL>* mMetaData; ///<points to metadata that all entities with the same bitset share
        std::vector< ComponentHandle > mComponentHandles; ///<stores indices of assigned component in their managers

        ComponentManager< CINDEX, COMP_TOTAL>& mComponentManager;

        void updateMetaData(CINDEX i, bool val);

    public:
        //forbit copying of an entity
        Entity(const Entity& e) = delete;
        Entity & operator=(const Entity&) = delete;

        ~Entity();
    };

    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    class Universe
    {

    };


    ///IMPLEMENTATION

    template<typename CINDEX, CINDEX COMP_TOTAL>
    EntityData<CINDEX, COMP_TOTAL>::EntityData(std::bitset< COMP_TOTAL > initialMask) : mSharedCount(0)
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
    bool Entity<CINDEX, COMP_TOTAL>::add(PARAM&&... param)
    {
        if (!has<C>()) //dont add if such a component is already assigned
        {
            updateMetaData(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), true);

            ComponentHandle c = mComponentManager.template assignComponent<C, PARAM...>( std::forward<PARAM>(param)... );

            //get the index we have to add the handle in
            auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            mComponentHandles.insert(mComponentHandles.begin() + handleIndex, c);
            return true;
        }
        return false;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Entity<CINDEX, COMP_TOTAL>::get() const
    {
        auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
        return mComponentManager.template getComponent<C>( mComponentHandles[handleIndex] );
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Entity<CINDEX, COMP_TOTAL>::modify()
    {
        auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
        return mComponentManager.template getComponent<C>( mComponentHandles[handleIndex] );
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    void Entity<CINDEX, COMP_TOTAL>::rem()
    {
        if (has<C>())
        {
            auto handleIndex = mMetaData->mMetaData[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ];
            mComponentManager.destroy( ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), mComponentHandles[handleIndex] );
            mComponentHandles.erase( mComponentHandles.begin() + handleIndex );
            updateMetaData(ComponentTraits<C, CINDEX, COMP_TOTAL>::getID(), false);
        }
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    void Entity<CINDEX, COMP_TOTAL>::updateMetaData(CINDEX i, bool val)
    {
        static std::unordered_map< unsigned long long, std::unique_ptr<EntityData<CINDEX, COMP_TOTAL>> > sEntityData;

        if (mMetaData)
        { //deref to old metadata
            mMetaData->mSharedCount--;
            if (mMetaData->mSharedCount == 0) //no entities with the current bitset anymore, can remove metadata
            {
                auto hashval = mComponentMask.to_ullong();
                sEntityData.erase(hashval);
            }
        }

        mComponentMask.set(i, val); //set the bit

        auto hashval = mComponentMask.to_ullong(); //get the hash value for the new bitset
        auto meta = sEntityData.emplace( hashval, std::unique_ptr<EntityData<CINDEX, COMP_TOTAL>>() ); //find metadata, may construct new
        if (meta.second)
            meta.first->second.reset( new EntityData<CINDEX, COMP_TOTAL>(mComponentMask) );
        mMetaData = meta.first->second.get(); //connect to the metadata
        mMetaData->mSharedCount++;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    std::shared_ptr< Entity<CINDEX, COMP_TOTAL> > Entity<CINDEX, COMP_TOTAL>::create(ComponentManager< CINDEX, COMP_TOTAL>& componentManager)
    {
        struct makeSharedEnabler : public Entity<CINDEX, COMP_TOTAL>
        {
            makeSharedEnabler(ComponentManager< CINDEX, COMP_TOTAL>& componentManager) : Entity<CINDEX, COMP_TOTAL>(componentManager){}
        };
        return std::make_shared<makeSharedEnabler>(componentManager);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    Entity<CINDEX, COMP_TOTAL>::~Entity()
    {
        for(CINDEX i = 0; i < COMP_TOTAL; ++i)
        {
            if (mComponentMask.test(i))
            {
                auto handleIndex = mMetaData->mMetaData[ i ];
                mComponentManager.destroy( i, mComponentHandles[handleIndex] );
            }
        }
    }
}

#endif // DOM_ENTITY_H

