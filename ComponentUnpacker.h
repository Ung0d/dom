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

#ifndef COMPONENT_UNPACKER_H
#define COMPONENT_UNPACKER_H

#include <vector>
#include "ChunkedArray.h"

namespace dom
{
    using ComponentHandle = ElementHandle;

    template <typename CINDEX, CINDEX COMP_TOTAL>
    class EntityManager;

    template <typename CINDEX, CINDEX COMP_TOTAL, typename... C>
    struct ComponentUnpacker;

    template <typename CINDEX, CINDEX COMP_TOTAL, typename C1, typename... C>
    struct ComponentUnpacker<CINDEX, COMP_TOTAL, C1, C...>
    {
        static void unpack(std::vector< std::pair<CINDEX, bool> >& vec,
                           std::vector< ComponentHandle >& comphandles,
                           EntityManager<CINDEX, COMP_TOTAL>& cm)
        {
            comphandles.push_back( cm.template assignComponent<C1>() );
            vec.emplace_back( ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID(), true );
            ComponentUnpacker<CINDEX, COMP_TOTAL, C...>::unpack(vec, comphandles, cm);
        }
    };

    template <typename CINDEX, CINDEX COMP_TOTAL, typename C1>
    struct ComponentUnpacker<CINDEX, COMP_TOTAL, C1>
    {
        static void unpack(std::vector< std::pair<CINDEX, bool> >& vec,
                           std::vector< ComponentHandle >& comphandles,
                           EntityManager<CINDEX, COMP_TOTAL>& cm)
        {
            comphandles.push_back( cm.template assignComponent<C1>() );
            vec.emplace_back( ComponentTraits<C1, CINDEX, COMP_TOTAL>::getID(), true );
        }
    };
}

#endif // COMPONENT_UNPACKER_H
