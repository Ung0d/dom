//#include "Entity.h"
#include "ComponentTraits.h"
#include "ChunkedArray.h"
#include "Entity.h"
#include "ComponentManager.h"

#define BOOST_TEST_MODULE EntityTesting
#include <boost/test/unit_test.hpp>
#include <iostream>


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
    dom::ElementHandle h = pos.add(0.2f,5.0f);
    BOOST_CHECK_EQUAL(pos.get(h).x, 0.2f);
    pos.destroy(h);

    //multielement test
    dom::ElementHandle h1 = pos.add(0.2f,5.0f);
    dom::ElementHandle h2 = pos.add(4.4f,3.0f);
    dom::ElementHandle h3 = pos.add(100.0f,100.0f);
    BOOST_CHECK_EQUAL(pos.get(h1).x, 0.2f);
    BOOST_CHECK_EQUAL(pos.get(h2).x, 4.4f);
    BOOST_CHECK_EQUAL(pos.get(h3).x, 100.0f);
    pos.destroy(h2);
    dom::ElementHandle h4 = pos.add(0.0f,0.0f);
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
    try
    {
        std::cout << "Testing error message for too many components." << std::endl;
        dom::ComponentTraits<TooMuch, unsigned char, 2>::getID();
    }
    catch(const dom::ComponentCountError& e)
    {
        std::cout << e.what() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE( test1 )
{
    struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };

    dom::ComponentManager<> cm;

    std::shared_ptr<dom::Entity<>> e = dom::Entity<>::create(cm);
    e->add<Position>(33,0);
    BOOST_CHECK_EQUAL( e->get<Position>().x, 33 );
}
