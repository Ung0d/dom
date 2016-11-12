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

#ifndef CREATE_AT_ONCE_H
#define CREATE_AT_ONCE_H

namespace dom
{
    template<typename CINDEX, CINDEX COMP_TOTAL, typename ...C>
    std::vector<Entity<CINDEX, COMP_TOTAL>> createAtOnce(std::size_t n, EntityManager< CINDEX, COMP_TOTAL>& cm)
    {
        //prepare a sample bitset
        std::bitset<COMP_TOTAL> sampleMask;
        std::vector< std::pair<CINDEX, bool> > vec;
        std::vector< ComponentHandle > comphandles;
        ComponentUnpacker<CINDEX, COMP_TOTAL, C...>::unpack(vec, comphandles, cm);
        for (CINDEX i = 0; i < vec.size(); ++i)
        {
            sampleMask.set(vec[i].first);
        }
        auto hashval = sampleMask.to_ullong(); //get the hash value for the new bitset
        auto meta = Entity<CINDEX, COMP_TOTAL>::getEntityData().emplace( hashval, std::unique_ptr<typename Entity<CINDEX, COMP_TOTAL>::EntityData>() ); //find metadata, may construct new
        if (meta.second)
            meta.first->second.reset( new typename Entity<CINDEX, COMP_TOTAL>::EntityData(sampleMask) );

        //prepare the vector
        std::vector<Entity<CINDEX, COMP_TOTAL>> entities;
        entities.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            entities.emplace_back(cm);
            std::vector< std::pair<CINDEX, bool> > vec;
            std::vector< ComponentHandle > comphandles;
            ComponentUnpacker<CINDEX, COMP_TOTAL, C...>::unpack(vec, comphandles, cm);
            entities.back().add( meta.first->second.get(), vec, comphandles );
        }

        return entities;
    }
}

#endif
