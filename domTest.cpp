#include "dom.h"

#define BOOST_TEST_MODULE EntityTesting
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
    pos.destroy(h);

    //multielement test
    dom::ChunkedArrayHandle h1 = pos.add(0.2f,5.0f);
    dom::ChunkedArrayHandle h2 = pos.add(4.4f,3.0f);
    dom::ChunkedArrayHandle h3 = pos.add(100.0f,100.0f);
    BOOST_CHECK_EQUAL(pos.get(h1).x, 0.2f);
    BOOST_CHECK_EQUAL(pos.get(h2).x, 4.4f);
    BOOST_CHECK_EQUAL(pos.get(h3).x, 100.0f);
    pos.destroy(h2);
    dom::ChunkedArrayHandle h4 = pos.add(0.0f,0.0f);
    BOOST_CHECK_EQUAL(h2.block, h4.block);
    BOOST_CHECK_EQUAL(h2.index, h4.index);
    pos.destroy(h1);
    pos.destroy(h3);
    pos.destroy(h4);
    h1 = pos.add(0.2f,5.0f);
    h2 = pos.add(4.4f,3.0f);
    h3 = pos.add(100.0f,100.0f);
    h4 = pos.add(100.0f,100.0f);
    BOOST_CHECK_EQUAL(pos.blockCount(), 2u);
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
    e.add<Position>();
    e.modify<Position>().x = 3;
    BOOST_CHECK_EQUAL(3, e.get<Position>().x);
    BOOST_REQUIRE(e.valid());
    BOOST_REQUIRE(e2.valid());
    e.destroy();
    BOOST_REQUIRE(!e.valid());
    BOOST_REQUIRE(!e2.valid());
    }

    //creat entity, assign 3 components, destroy
    {
    dom::EntityHandle<> e = universe.create();
    e.add<Position>();
    e.add<Gravity>();
    e.add<Velocity>();
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
    e.destroy();
    }

    //creat entity, assign 3 components at once, destroy
    {
    dom::EntityHandle<> e = universe.create<Position, Gravity, Velocity>();
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
    e.destroy();
    }

    //test dom::instantiate
    {
    dom::EntityHandle<> e = universe.create();
    e.add( universe.instantiate<Position>(5,5),
           universe.instantiate<Gravity>(1) );
    BOOST_REQUIRE(e.has<Position>());
    BOOST_REQUIRE(e.has<Gravity>());
    BOOST_CHECK_EQUAL(5, e.get<Position>().x);
    BOOST_CHECK_EQUAL(1, e.get<Gravity>().grav);
    e.destroy();
    }
}
