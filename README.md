# dom
An implementation of the Entity-Component-System pattern.
Requirements: - c++14 compiler

All starts with an entity. Very object in the "world" of your real-time-application is an entity.
By itself, an entity is just an empty container, completely useless. In order to give your entities
data, you have to assign components to it. Dom handles entities with shared_ptrs.

A quick how to so far:

At the very beginning, we need to create a ComponentManager and use it to create entities.
```
dom::ComponentManager<> cm;
std::shared_ptr<dom::Entity<>> e = dom::Entity<>::create(cm);
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
