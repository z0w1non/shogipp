#include "shogipp.hpp"
#include <chrono>

int main(int ac, char ** av)
{
    using namespace shogipp;
    do_taikyoku<niwatori_evaluator_t, random_evaluator_t>(true);
}