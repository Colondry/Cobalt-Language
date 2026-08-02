#include "chart.hpp"
#include "../sdl3_win64/internal.hpp"
#include "../sdl3_win64/sdl3_win64.hpp"
#include <SDL3/SDL.h>

#include <algorithm>

using namespace sdlint;

__Chart__ Chart;

void __Chart__::Bar(
    int x,
    int y,
    int width,
    int height,
    const std::vector<int>& data
)
{
    if (data.empty()) return;
    RGB rgb = hexStringToRgb(Chart.Color);
    int maxValue =
        *std::max_element(
            data.begin(),
            data.end()
        );

    float barWidth =
        (float)width / data.size();

    for (size_t i = 0;i < data.size();i++)
    {
        float ratio =
            (float)data[i] / maxValue;

        float barHeight =
            ratio * height;

        SDL_FRect rect =
        {
            x + i * barWidth,
            y + height - barHeight,
            barWidth - 4,
            barHeight
        };

        SDL_SetRenderDrawColor(
            renderer,
            70,
            170,
            255,
            255
        );

        SDL_RenderFillRect(
            renderer,
            &rect
        );

        SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);

        SDL_RenderDebugText(
            renderer,
            x,
            y - 30,
            Chart.Title.c_str()
        );
    }
}
void __Chart__::Line(
    int x,
    int y,
    int width,
    int height,
    const std::vector<int>& data)
{
    if (data.size() < 2) return;
    RGB rgb = hexStringToRgb(Chart.Color);

    int max =
        *std::max_element(
            data.begin(),
            data.end());

    float step =
        (float)width / (data.size() - 1);

    SDL_SetRenderDrawColor(
        renderer,
        rgb.r,
        rgb.g,
        rgb.b,
        255);

    for (size_t i = 0;i < data.size() - 1;i++)
    {
        float x1 =
            x + i * step;

        float y1 =
            y + height -
            ((float)data[i] / max) * height;

        float x2 =
            x + (i + 1) * step;

        float y2 =
            y + height -
            ((float)data[i + 1] / max) * height;

        SDL_RenderLine(
            renderer,
            x1,
            y1,
            x2,
            y2);
    }
    SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);

    SDL_RenderDebugText(
        renderer,
        x,
        y - 30,
        Chart.Title.c_str()
    );
}

__Table__ Table;

void __Table__::Begin(int x, int y)
{
    startX = x;
    currentY = y;
}

void __Table__::Header(
    const std::string& c1,
    const std::string& c2)
{
    Header({ c1, c2 });
}

void __Table__::Header(
    std::initializer_list<std::string> cols)
{
    int x = startX;

    // SDL_RenderDebugText draws using the *current* draw color. The only
    // other place that sets it is Window::BackColor(), which is the
    // background clear color -- without setting it here explicitly, the
    // header text renders in the same color as the background and is
    // invisible even though it's being drawn every frame.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (auto& c : cols)
    {
        SDL_RenderDebugText(
            renderer,
            x,
            currentY,
            c.c_str());

        x += 150;
    }

    currentY += 25;
}

void __Table__::Row(
    const std::string& c1,
    const std::string& c2)
{
    Row({ c1, c2 });
}

void __Table__::Row(
    const std::string& c1,
    double value)
{
    Row({ c1, std::to_string(value) });
}

void __Table__::Row(
    std::initializer_list<std::string> cols)
{
    int x = startX;

    // See Header() above -- same invisible-text bug, same fix.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (auto& c : cols)
    {
        SDL_RenderDebugText(
            renderer,
            x,
            currentY,
            c.c_str());

        x += 150;
    }

    currentY += 20;
}

void __Table__::End()
{
}