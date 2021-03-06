# dom
An implementation of the Entity-Component-System pattern. Worldobjects are modelled through entities.
Every piece of data is modelled through a component. Components are compile-time known.
Each entity can (but must not) have every component at most one time.
Components are stored semi-continously in memory. There are no "gaps" in the memory for
sparsly assigned components. Entities are highly space- and runtime-efficient.

Requirements: - c++14 compiler

Dom is single-file and header only. Just include "dom.h" to your project.

Before using dom, you may want to take 3 minutes to take a quick look on the features:

At the very beginning, you may want to finetune the integer type, that is used as component-index
and the maximum number of components for your application. You can of corse just use the default
arguments unsigned char (8bits) and 64.

Note that using and integer type with more bits and/or a higher number of max-components will
rise the memory-usage of your application. 

```
using Universe = dom::Universe<>;
using Entity = dom::EntityHandle<>;
/* OR for example
* using Universe = dom::Universe<unsigned char, 128>;
* using Entity = dom::EntityHandle<unsigned char, 128>;
*/
```

Every object in the "world" of your real-time-application is an entity.
By itself, an entity is just an empty container. In order to give your entities
data, you have to assign components to it. 
Entities (and Components) are stored in the dom::Universe<> datastructure. You must use the universe
to create and destroy entities. 

```
//using the typedefs above for readability
Universe universe;
Entity e = universe.create();
```

You can destroy entities with by calling `e.destroy()`. This will automatically invalidate all other
entity-handles pointing to that entity. You can check whether an entity-handle is still valid with
`e.valid()` or just `if(e) ... `.
Note that every created entity must be deleted manually. Destroying all handles of an entity DOES NOT
destroy the entity. If you destroy every handle of an entity which you did not destroy before, you
will have memory leaks in your application.
[The above design decision is dedicated to the application speed. Using shared_ptr-like handles, which
destroy the entity automatically, will slow down the application and rises the space for each handle.]

Now we need a arbitrary struct that models one of our components. Note that components should not have logic.
As you can see, the component-structs in dom dont need to derive from something. Just the bare struct.
Our struct has, as you can see, a default constructor and a constructor with arguments. Both is possible.
Default-constructability is not required.
```
struct Position
    {
        Position() : x(0), y(0) {}
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };
```

And we add it to the entitiy e like this:
```
e.add<Position>();
//or if you want to use a non-default-constructor
//e.add( universe.instantiate<Position>(3,60) );
```

You can also add multiple components at once to an entity. Whenever possible you should prefer this over the
one-by-one-assignment, since it is faster. In the following code-snipped Gravity and Velocity are assumed to
be component-structs like Position with the appropriate constructors.
```
e.add<Position, Gravity, Velocity>();
//or if you want to use a non-default-constructors
//e.add( universe.instantiate<Position>(3,60),
          universe.instantiate<Gravity>(-1),
          universe.instantiate<Velocity>(1,1));
```

If an entity has a specific component, you can access it like this:
```
if (e.has<Position>()) //returns true if e has a Position Component
{
    e.get<Position>(); //returns const-reference to the Position struct of entity e
    
    e.modify<Position>(); //returns non-const-reference to the Position struct of entity e
}
e.rem<Position>(); //removes the Position component from e (calls destructor of the struct)
```

If you want to create multiple entities with the same components, the fastest way of doing so is:
```
//creates 1000 entities with Position and Velocity components and
//push them in a vector
std::vector<Entity> vec;
universe.create(1000, [&vec] (Entity en) { vec.emplace_back(en); });
```

Last but not least: If you have a datastructure containing entities and want to iterate over all
entities with a specific set of components, use:
```
dom::Utility<>::iterate<Position, Velocity>(e, [](Entity e, Position &position, Velocity &velocity)
{
   position.x += velocity.x;
   position.y += velocity.y;
});
```

Sometimes its desirable to have the possibility to attach multiple components of one type to an entity. 
To do this, your Component has to derive from `dom::MultiComponent` interface. See domTest.cpp for an example.
Note that this has zero overhead impact on the overall system. Components are normally intended to be naturally 
exclusive per entity. Using `dom::Multicomponent` however will give the components that derive from it a small overhead, 
since every component stores a handle to the next component (in single-linked-list manner).
