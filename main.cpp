#include "dom.h"
//#include <entityx/entityx.h>

#include <iostream>
#include <chrono>
#include <list>

using Universe = dom::Universe<>;
using Entity = dom::EntityHandle<>;

const unsigned num = 1000000;

int main()
{
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

    //for (const auto& en: e)
    //en.destroy();
    //e.clear();

   /*std::cout<< std::endl << "checking for entityx" << std::endl;

    struct MovementSystem : public entityx::System<MovementSystem>
    {
      void update(entityx::EntityManager &es, entityx::EventManager &events, entityx::TimeDelta dt) override
      {
        es.each<Position, Velocity>([dt](entityx::Entity entity, Position &position, Velocity &direction)
        {
          position.x += direction.x;
          position.y += direction.y;
        });
      };
    };

    class Level : public entityx::EntityX
    {
    public:
      explicit Level()
      {
        systems.add<MovementSystem>();
        systems.configure();

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < num; ++i)
        {
            e2.emplace_back(entities.create());
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << num << " empty entities created in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;

        begin = std::chrono::steady_clock::now();
        for (auto& e : e2)
        {
            e.assign<Position>(33,0);
            e.assign<Velocity>(1,1);
        }
        end = std::chrono::steady_clock::now();
        std::cout << "Components assigned in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;
      }

      void update(entityx::TimeDelta dt)
      {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        systems.update<MovementSystem>(dt);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Iterated over all components in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << std::endl << std::endl;
      }

        std::list<entityx::Entity> e2;
    };

    Level level;
    level.update(0.1);*/

    return 0;
}

