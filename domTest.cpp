<<<<<<< HEAD
#define BOOST_TEST_MODULE EntityTesting
#include "dom.h"
=======
#include "dom.h"

#define BOOST_TEST_MODULE EntityTesting
>>>>>>> 213ce508ea7dd654dc4f8dfae02edb251363ef79
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <chrono>


BOOST_AUTO_TEST_CASE( chunked_array_test )
{
    struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };

    dom::ChunkedArray<Position,3> pos;

    //single element test
    dom::ChunkedArrayHandle h = pos.add(0.2f,5.0f);
    BOOST_CHECK_EQUAL(pos.get(h).x, 0.2f);
    BOOST_CHECK_EQUAL(pos.size(), 1u);   //check whether element is registered by size counter
    pos.destroy(h);
    BOOST_CHECK_EQUAL(pos.size(), 0u);   //check whether element is degeristed by size counter

    //multielement test
    dom::ChunkedArrayHandle h1 = pos.add(0.2f,5.0f);
    dom::ChunkedArrayHandle h2 = pos.add(4.4f,3.0f);
    dom::ChunkedArrayHandle h3 = pos.add(100.0f,100.0f);
    BOOST_CHECK_EQUAL(pos.get(h1).x, 0.2f);
    BOOST_CHECK_EQUAL(pos.get(h2).x, 4.4f);
    BOOST_CHECK_EQUAL(pos.get(h3).x, 100.0f);
    BOOST_CHECK_EQUAL(pos.size(), 3u);
    pos.destroy(h2);
    dom::ChunkedArrayHandle h4 = pos.add(0.0f,0.0f);
    BOOST_CHECK_EQUAL(pos.size(), 3u);
    BOOST_CHECK_EQUAL(h2.block, h4.block);
    BOOST_CHECK_EQUAL(h2.index, h4.index);
    pos.destroy(h1);
    pos.destroy(h3);
    pos.destroy(h4);
    BOOST_CHECK_EQUAL(pos.size(), 0u);
    h1 = pos.add(0.2f,5.0f);
    h2 = pos.add(4.4f,3.0f);
    h3 = pos.add(100.0f,100.0f);
    h4 = pos.add(100.0f,100.0f);
    BOOST_CHECK_EQUAL(pos.blockCount(), 2u);
    BOOST_CHECK_EQUAL(pos.size(), 4u);
    pos.destroy(h1);
    pos.destroy(h2);
    pos.destroy(h3);
    pos.destroy(h4);

    //check for leaks
    BOOST_CHECK_EQUAL(pos.size(), 0u);
}
BOOST_AUTO_TEST_CASE( component_id_test )
{
    struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };
    struct Size
    {
        Size(float cwidth, float cheight) : width(cwidth), height(cheight) {}

        float width;
        float height;
    };
    struct TooMuch
    {
    };

    auto i1 = dom::ComponentTraits<Position, unsigned char, 2>::getID();
    auto i2 = dom::ComponentTraits<Size, unsigned char, 2>::getID();
    BOOST_CHECK_EQUAL( i1, 0 );
    BOOST_CHECK_EQUAL( i2, 1 );
    bool check = false;
    try
    {
        dom::ComponentTraits<TooMuch, unsigned char, 2>::getID();
    }
    catch(const dom::ComponentCountError& e)
    {
        check = true;
    }
    BOOST_REQUIRE(check);
}

BOOST_AUTO_TEST_CASE( single_entity )
{
    struct Position
    {
        float x;
        float y;

        Position() {}
        Position(float cx, float cy) : x(cx), y(cy) {}
    };

    struct Velocity
    {
        float x;
        float y;
    };

    struct Gravity
    {
        float grav;

        Gravity() {}
        Gravity(float cgrav) : grav(cgrav) {}
    };

    dom::Universe<> universe;

    //creat eentity, assign single component, destroy
    {
    dom::EntityHandle<> e = universe.create();
    dom::EntityHandle<> e2 = e;
    BOOST_CHECK_EQUAL(universe.getEntityCount(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);
    e.add<Position>();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 1u);
    e.modify<Position>().x = 3;
    BOOST_CHECK_EQUAL(3, e.get<Position>().x);
    BOOST_REQUIRE(e.valid());
    BOOST_REQUIRE(e2.valid());
    e.destroy();
    BOOST_CHECK_EQUAL(universe.getEntityCount(), 0u);  //check for memory leaks
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u); //check for memory leaks
    BOOST_REQUIRE(!e.valid());
    BOOST_REQUIRE(!e2.valid());
    }

    //creat entity, assign 3 components, destroy
    {
    dom::EntityHandle<> e = universe.create();
    e.add<Position>();
    e.add<Gravity>();
    e.add<Velocity>();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Velocity>(), 1u);
    e.modify<Position>().x = 3;
    e.modify<Velocity>().x = 5;
    e.modify<Gravity>().grav = 2;
    BOOST_CHECK_EQUAL(3, e.get<Position>().x);
    BOOST_CHECK_EQUAL(5, e.get<Velocity>().x);
    BOOST_CHECK_EQUAL(2, e.get<Gravity>().grav);
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Velocity>());
    BOOST_REQUIRE(e.has<Gravity>());
    e.rem<Gravity>();
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    e.rem<Velocity>();
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(!e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    e.rem<Position>();
    BOOST_REQUIRE(!e.has<Position>());
    BOOST_REQUIRE(!e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Velocity>(), 0u);
    e.destroy();
    }

    //creat entity, assign 3 components at once, destroy
    {
    dom::EntityHandle<> e = universe.create<Position, Gravity, Velocity>();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Velocity>(), 1u);
    e.modify<Position>().x = 3;
    e.modify<Velocity>().x = 5;
    e.modify<Gravity>().grav = 2;
    BOOST_CHECK_EQUAL(3, e.get<Position>().x);
    BOOST_CHECK_EQUAL(5, e.get<Velocity>().x);
    BOOST_CHECK_EQUAL(2, e.get<Gravity>().grav);
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Velocity>());
    BOOST_REQUIRE(e.has<Gravity>());
    e.rem<Gravity>();
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    e.rem<Velocity>();
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(!e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    e.rem<Position>();
    BOOST_REQUIRE(!e.has<Position>());
    BOOST_REQUIRE(!e.has<Velocity>());
    BOOST_REQUIRE(!e.has<Gravity>());
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Velocity>(), 0u);
    e.destroy();
    }

    //test dom::instantiate
    {
    dom::EntityHandle<> e = universe.create();
    e.add( universe.instantiate<Position>(5,5),
           universe.instantiate<Gravity>(1) );
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 1u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 1u);
    e.add( universe.instantiate<Position>(5,5),
           universe.instantiate<Gravity>(1) );   //this invoke must be ignored since components exists already
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 1u);  //check for memory leaks
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 1u);   //check for memory leaks
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Gravity>());
    BOOST_CHECK_EQUAL(5, e.get<Position>().x);
    BOOST_CHECK_EQUAL(1, e.get<Gravity>().grav);
    e.destroy();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount<Gravity>(), 0u);
    }
}


BOOST_AUTO_TEST_CASE( test_multi_component )
{
    struct Position
    {
        float x;
        float y;

        Position() {}
        Position(float cx, float cy) : x(cx), y(cy) {}
    };

    dom::Universe<> universe;

    dom::EntityHandle<> e = universe.create();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);
    BOOST_CHECK_EQUAL(universe.getComponentCount< dom::MultiComponent<Position> >(), 0u);
    e.add< dom::MultiComponent<Position> >( universe.instantiate< dom::MultiComponent<Position> >(7, universe, 0.0f, 0.0f) );
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 7u);
    BOOST_CHECK_EQUAL(universe.getComponentCount< dom::MultiComponent<Position> >(), 1u);
    e.add< dom::MultiComponent<Position> >(universe.instantiate< dom::MultiComponent<Position> >(2, universe, 0.0f, 0.0f)); //these additional component instantiations must be deleted
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 7u);  //check for memory leaks
    BOOST_CHECK_EQUAL(universe.getComponentCount< dom::MultiComponent<Position> >(), 1u);

    BOOST_REQUIRE( !e.has< Position >() );
    BOOST_REQUIRE( e.has< dom::MultiComponent<Position> >() );
    BOOST_CHECK_EQUAL( e.get< dom::MultiComponent<Position> >().getComponent(0).x, 0.0f );
    BOOST_CHECK_EQUAL( e.get< dom::MultiComponent<Position> >().getComponent(4).x, 0.0f );
    dom::MultiComponent<Position>& p = e.modify<dom::MultiComponent<Position>>();
    BOOST_CHECK_EQUAL(p.getComponentCount(), 7u);

    e.destroy();
    BOOST_CHECK_EQUAL(universe.getComponentCount<Position>(), 0u);  //check for memory leaks
    BOOST_CHECK_EQUAL(universe.getComponentCount< dom::MultiComponent<Position> >(), 0u);
}

struct TestMe
{
    static int dcounter;

    float f;
    bool b;
    std::string s;

    ~TestMe() {  dcounter++; }
};

int TestMe::dcounter = 0;

BOOST_AUTO_TEST_CASE( test_move_insert )
{
    {
    TestMe test;
    test.f = 1.0f;
    test.b = true;
    test.s = "test";

    dom::ChunkedArray<TestMe> ca;
    auto h = ca.add(test);  //add by copy
    BOOST_CHECK_EQUAL(ca.get(h).s, "test");
    ca.destroy(h);
    }
    BOOST_CHECK_EQUAL(TestMe::dcounter, 2);
}


BOOST_AUTO_TEST_CASE( test_attach_order )
{
    struct Position
    {
        float x;
        float y;

        Position() : x(0), y(0) {}
        Position(float cx, float cy) : x(cx), y(cy) {}
    };

    struct Velocity
    {
        float x;
        float y;
        Velocity() : x(0), y(0) {}
    };

    struct Gravity
    {
        float grav;

        Gravity() {}
        Gravity(float cgrav) : grav(cgrav) {}
    };

    dom::Universe<> universe;
    dom::EntityHandle<> e = universe.create<Position, Velocity>();
    dom::EntityHandle<> e2 = universe.create<Gravity, Velocity, Position>();
    BOOST_CHECK_EQUAL(e2.get<Velocity>().x, 0);
    BOOST_CHECK_EQUAL(e2.get<Position>().x, 0);
    BOOST_CHECK_EQUAL(e.get<Velocity>().x, 0);
    BOOST_CHECK_EQUAL(e.get<Position>().x, 0);
}

BOOST_AUTO_TEST_CASE( entity_copy )
{
    using Universe = dom::Universe<>;
    using Entity = dom::EntityHandle<>;

    struct TestComponent
    {
        TestComponent(float cx) : x(cx) {}
        TestComponent() : x(0) {}

        float x;
    };

    Universe universe;
    Entity e = universe.create<TestComponent>();
    e.modify<TestComponent>().x = 10.0f;
    Entity ecopy = e.copy<TestComponent>();
    BOOST_REQUIRE(ecopy.has<TestComponent>());
    BOOST_CHECK_EQUAL(ecopy.get<TestComponent>().x, 10.0f);
}

BOOST_AUTO_TEST_CASE( speed_test )
{
    using Universe = dom::Universe<>;
    using Entity = dom::EntityHandle<>;

    const unsigned num = 1000000;


    struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}
        Position() : x(0), y(0) {}

        float x;
        float y;
    };
    struct Velocity
    {
        Velocity(float cx, float cy) : x(cx), y(cy) {}
        Velocity() : x(1), y(1) {}

        float x;
        float y;
    };


    Universe universe;
    std::list<Entity> e;

    std::cout << "Checking for dom" << std::endl;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < num; ++i)
    {
        e.emplace_back(universe.create());
        e.back().add<Position>();
        e.back().add<Velocity>();
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << num << " entities with components created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    for (auto& entity : e)
    {
        entity.destroy();
    }
    e.clear();

    std::cout<< std::endl << "checking for dom with improvements" << std::endl;

    begin = std::chrono::steady_clock::now();
    universe.create<Position, Velocity>(num, [&e] (Entity en) { e.emplace_back(en); });
    end = std::chrono::steady_clock::now();
    std::cout << num << " entities with components created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    begin = std::chrono::steady_clock::now();
    dom::Utility<Entity>::iterate<Position, Velocity>(e, [](Entity e, Position &position, Velocity &direction)
        {
          position.x += direction.x;
          position.y += direction.y;
        });
    end = std::chrono::steady_clock::now();
    std::cout << "Iterated over all components in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;
}
