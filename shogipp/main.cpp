#include "shogipp.hpp"
#include <chrono>

int main(int ac, char ** av)
{
    using namespace shogipp;
    do_game<hiyoko_evaluator_t, random_evaluator_t>(true);
}