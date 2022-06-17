#include "shogipp.hpp"

int main(int ac, char ** av)
{
    using namespace shogipp;

    game_t game{ { std::make_shared<sample_evaluator_t>(), std::make_shared<random_evaluator_t>() } };
    game.init();
    while (game.procedure());
}