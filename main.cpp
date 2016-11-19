/*#include "dom.h"
#include <entityx/entityx.h>

#define BOOST_TEST_MODULE EntityTesting
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <chrono>

int main()
{
    std::cout << "Checking for dom" << std::endl;

    struct Position
    {
        Position() : x(0), y(0) {}

        float x;
        float y;
    };
    struct Velocity
    {
        Velocity() : x(1), y(1) {}

        float x;
        float y;
    };

    dom::Universe<> u;
    unsigned num = 1000000;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::vector<dom::Entity<>> e = dom::createAtOnce<unsigned char, 64, Position, Velocity>(num, cm);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << num << " entities created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    e.clear();

    begin = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < num; ++i)
    {
        e.emplace_back(cm);
    }

    for (unsigned i = 0; i < num; ++i)
    {
        e[i].add<Position, Velocity>();
    }
    end = std::chrono::steady_clock::now();
    std::cout << num << " entities created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    begin = std::chrono::steady_clock::now();
    dom::Utility<>::iterate<Position, Velocity>(e, [] (const dom::Entity<>& e, Position& p, Velocity& v)
     {
         p.x += v.x;
         p.y += v.y;
     });
    end = std::chrono::steady_clock::now();
    std::cout << "Iterated over all components in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    std::cout << e[1000].get<Position>().x << std::endl;






    /*std::cout<< std::endl << "checking for entityx" << std::endl;

    entityx::EntityX ex;
    std::vector<entityx::Entity> e2;
    e2.reserve(num);

    entityx::Entity entity = ex.entities.create();
    begin = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < num; ++i)
    {
        e2.emplace_back(ex.entities.create());
    }
    end = std::chrono::steady_clock::now();
    std::cout << num << " entities created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    begin = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < num; ++i)
    {
        e2[i].assign<Position>(33,0);
        e2[i].assign<Velocity>(1,1);
    }
    end = std::chrono::steady_clock::now();
    std::cout << "Components assigned in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

    struct MovementSystem : public entityx::System<MovementSystem>
    {
      void update(entityx::EntityManager &es, entityx::EventManager &events, entityx::TimeDelta dt) override
      {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        es.each<Position, Velocity>([dt](Entity entity, Position &position, Velocity &direction)
        {
          position.x += direction.x;
          position.y += direction.y;
        });
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Iterated over all components in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;
      };
    };*/

    //std::cout << e2[1000].component<Position>().x << std::endl;

    /*return 0;
}*/
