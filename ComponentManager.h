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

#ifndef COMPONENT_MANAGER_H
#define COMPONENT_MANAGER_H

#include "ChunkedArray.h"
#include "ComponentTraits.h"

namespace dom
{
    template<typename CINDEX, CINDEX COMP_TOTAL> class Entity;
    using ComponentHandle = ElementHandle;

    /**
    * \brief Manager class for ALL components in the application.
    * Components of the same type C are stored in semi-contingous space.
    * This class provides functionality to create, destroy and retrieve
    * components given a type C and a ComponentHandle.
    */
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    class ComponentManager
    {
    private:
        std::array< std::unique_ptr<BaseChunkedArray>, COMP_TOTAL> mManagers;

    public:
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
    };



    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename ...PARAM>
    ComponentHandle ComponentManager<CINDEX, COMP_TOTAL>::assignComponent(PARAM&&... param)
    {
        //create the corresponding container if not existing yet
        if (!mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ])
        {
            mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ] =
                std::unique_ptr<BaseChunkedArray>( new ChunkedArray<C>() );
        }
        ChunkedArray<C>* ca = static_cast<ChunkedArray<C>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->add( std::forward<PARAM>(param)... );
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& ComponentManager<CINDEX, COMP_TOTAL>::getComponent(ComponentHandle h)
    {
        ChunkedArray<C>* ca = static_cast<ChunkedArray<C>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->get(h);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& ComponentManager<CINDEX, COMP_TOTAL>::getComponent(ComponentHandle h) const
    {
        ChunkedArray<C>* ca = static_cast<ChunkedArray<C>*>( mManagers[ ComponentTraits<C, CINDEX, COMP_TOTAL>::getID() ].get() );
        return ca->get(h);
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    void ComponentManager<CINDEX, COMP_TOTAL>::destroy(CINDEX cid, ComponentHandle h)
    {
        mManagers[ cid ].get()->destroy(h);
    }
}

#endif // COMPONENT_MANAGER_H
