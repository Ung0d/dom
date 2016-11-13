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

#ifndef DOM_UTILITY_H
#define DOM_UTILITY_H

#include <functional>
#include <iostream>
#include "Entity.h"

namespace dom
{
    template<typename CINDEX = unsigned char, CINDEX COMP_TOTAL = 64>
    struct Utility
    {
        template <typename T> struct Identity { typedef T type; };

        template <typename... C>
        struct ComponentChecker;

        template <typename ... C>
        static void iterate( std::vector<Entity<CINDEX, COMP_TOTAL>>& entities,
                      typename Identity<std::function<void(const Entity<CINDEX, COMP_TOTAL>&, C&...)>>::type f);
    };


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template <typename ... C>
    void Utility<CINDEX, COMP_TOTAL>::iterate(
                         std::vector<Entity<CINDEX, COMP_TOTAL>>& entities,
                         typename Identity<std::function<void(const Entity<CINDEX, COMP_TOTAL>&, C&...)>>::type f)
    {
        for (auto& e : entities)
        {
            if (ComponentChecker<C...>::check(e))
            {
                f(e, e.template modify<C>()... );
            }
        }
    }

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template <typename C1, typename... C>
    struct Utility<CINDEX, COMP_TOTAL>::ComponentChecker<C1, C...>
    {
      static bool check(const Entity<CINDEX, COMP_TOTAL>& e)
      {
          return e.template has<C1>() && Utility<CINDEX, COMP_TOTAL>::ComponentChecker<C...>::check(e);
      }
    };

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template <typename C1>
    struct Utility<CINDEX, COMP_TOTAL>::ComponentChecker<C1>
    {
      static bool check(const Entity<CINDEX, COMP_TOTAL>& e)
      {
        return e.template has<C1>();
      }
    };
}

#endif // DOM_UTILITY_H
