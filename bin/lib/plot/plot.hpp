#ifndef PLOT
#define PLOT

#include "matplotlibcpp.h"
#include <string>
#include <vector>

namespace plt = matplotlibcpp;

class __Plot__
{
public:
    std::string Title = "My Graph";
    std::string XLabel = "";
    std::string YLabel = "";
    bool Grid = false;
    std::string Color = "#000000";
    int LineWidth = 2;

    template<typename T>
    void Line(
        std::vector<T>& x,
        std::vector<T>& y)
    {
        std::map<std::string, std::string> kw;

        kw["color"] = Color;
        kw["linewidth"] = std::to_string(LineWidth);
        plt::plot(x, y, kw);
    };

    template<typename T>
    void Bar(...);

    template<typename T>
    void Scatter(...);

    void Show();
};

extern __Plot__ Plot;

#endif