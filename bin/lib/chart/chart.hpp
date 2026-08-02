#pragma once
#include <string>
#include <vector>
#include <initializer_list>
#include "../sdl3_win64/internal.hpp"
#include <SDL3/SDL.h>

#include <algorithm>
#include <string>
using namespace sdlint;

class __Chart__
{
public:
    std::string Title = "";
    std::string Color = "#FFFFFF";
    static void Bar(
        int x,
        int y,
        int width,
        int height,
        const std::vector<int>& data
    );
    static void Line(
        int x,
        int y,
        int width,
        int height,
        const std::vector<int>& data
    );
};

class __Table__
{
public:

    void Begin(int x, int y);

    void Header(
        const std::string& c1,
        const std::string& c2);

    void Header(
        std::initializer_list<std::string> cols);

    void Row(
        const std::string& c1,
        const std::string& c2);

    void Row(
        const std::string& c1,
        double value);

    void Row(
        std::initializer_list<std::string> cols);

    void End();

private:

    int startX;
    int currentY;
};

extern __Chart__ Chart;
extern __Table__ Table;