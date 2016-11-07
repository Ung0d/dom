# dom
An implementation of the Entity-Component-System pattern.
Requirements: - c++14 compiler

All starts with an entity. Very object in the "world" of your real-time-application is an entity.
By itself, an entity is just an empty container, completely useless. In order to give your entities
data, you have to assign components to it. Dom handles entities with shared_ptrs.

A quick how to so far:

At the very beginning, we need to create a ComponentManager.

''
dom::ComponentManager<> cm;
''
