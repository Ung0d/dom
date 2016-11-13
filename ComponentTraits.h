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

#ifndef DOM_COMPONENT_TRAITS_H
#define DOM_COMPONENT_TRAITS_H

#include <stdexcept>

namespace dom
{
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
}

#endif // DOM_COMPONENT_TRAITS_H
