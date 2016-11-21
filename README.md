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

Now we need a arbitrary struct that models one of our components. Note that components should not have logic.
As you can see, the component-structs in dom dont need to derive from something. Just the bare struct.
```
struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };
```

And we add it to the entitiy e like this:
```
e->add<Position>(33,0);
//now use e->get<Position>() to access the component
```
