#include "shogipp.hpp"

int main(int ac, char ** av)
{
    using namespace shogipp;

    for (int i = 0; i < 10; ++i)
    {
        game_t game{ { std::make_shared<random_evaluator_t>(), std::make_shared<random_evaluator_t>() } };
        game.init();
        while (game.procedure());
    }
}