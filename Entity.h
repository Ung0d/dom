/*
* This file is part of the dom-library - an implementation of the Entity-Component-System Pattern.
*/

#include <bitset>

namespace dom
{
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
        * component is assigned.
        */
        template<typename C>
        bool has() const;

        /**
        * \brief Adds a new component of type C to the entity, if no other component of that type was added before.
        * Returns success.
        */
        template<typename C, typename ...PARAM>
        bool add(PARAM... param);

        /**
        * \brief Gets a const reference to the requested component. Throws an "No-Component-Found"-error if
        * has<C>() is false when this method is called.
        */
        template<typename C>
        const C& get() const;

        /**
        * \brief Gets a reference to the requested component. Throws an "No-Component-Found"-error if
        * has<C>() is false when this method is called.
        */
        template<typename C>
        C& modify();

    private:
    std::bitset<COMP_TOTAL> mComponentMask;
    };


    ///IMPLEMENTATION

    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    bool Entity<CINDEX, COMP_TOTAL>::has() const
    {
        //todo
        return false;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C, typename ...PARAM>
    bool Entity<CINDEX, COMP_TOTAL>::add(PARAM... param)
    {
        //todo
        return false;
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    const C& Entity<CINDEX, COMP_TOTAL>::get() const
    {
        //todo
        return C(); //omg lol instant crash
    }


    template<typename CINDEX, CINDEX COMP_TOTAL>
    template<typename C>
    C& Entity<CINDEX, COMP_TOTAL>::modify()
    {
        //todo
        return C(); //omg lol instant crash
    }
}
