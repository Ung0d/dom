#include "Entity.h"

#define BOOST_TEST_MODULE EntityTesting
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( test1 )
{
    struct Position
    {
        Position(float cx, float cy) : x(cx), y(cy) {}

        float x;
        float y;
    };

    dom::Entity<> e;
    BOOST_CHECK( e.add<Position>(1.0f,10.0f) );
}
