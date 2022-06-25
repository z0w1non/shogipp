#include "shogipp.hpp"

int main(int ac, char ** av)
{
    using namespace shogipp;
    do_taikyoku<niwatori_dou_evaluator_t, random_evaluator_t>(true);
}