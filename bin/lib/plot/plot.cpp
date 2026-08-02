#include "matplotlibcpp.h"
#include "plot.hpp"
#include <string>
#include <vector>

namespace plt = matplotlibcpp;
__Plot__ Plot;

void __Plot__::Show()
{
    plt::title(Plot.Title);
    plt::xlabel(Plot.XLabel);
    plt::ylabel(Plot.YLabel);
    plt::grid(Plot.Grid);
    plt::show();
}