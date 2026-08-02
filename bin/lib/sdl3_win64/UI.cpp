#define _USE_MATH_DEFINES

#include "sdl3_win64.hpp"
#include <SDL3/sdl.h>
#include <cstdio>
#include <cstdint>
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>
#include "internal.hpp"
using namespace sdlint;

#include <map>
#include <tuple>
#include <sstream>
#include <iomanip>

std::string invertHexColor(const std::string& hexColor) {
    std::string hex = hexColor;
    if (hex[0] == '#') {
        hex.erase(0, 1);
    }

    // Convert hex string to integer
    unsigned int rgb = std::stoul(hex, nullptr, 16);

    // Invert the color using XOR
    unsigned int invertedRgb = 0xFFFFFF ^ rgb;

    // Convert back to hex string
    std::stringstream ss;
    ss << "#" << std::uppercase << std::setfill('0') << std::setw(6) << std::hex << invertedRgb;

    return ss.str();
}

__Draw__ Draw;

void __Draw__::FillOval(float x, float y, float w, float h) {
    float cx = x + w / 2.0f;
    float cy = y + h / 2.0f;
    float rx = w / 2.0f;
    float ry = h / 2.0f;

    if (rx <= 0.0f || ry <= 0.0f) return;

    // Loop through each vertical row in the height bounds
    for (int j = 0; j <= (int)h; j++) {
        float py = y + (float)j;
        float dy = py - cy;

        // Find the horizontal width limit at this Y coordinate using ellipse formula
        float ratio = 1.0f - (dy * dy) / (ry * ry);
        if (ratio < 0.0f) ratio = 0.0f;
        float dx = rx * SDL_sqrtf(ratio);

        SDL_RenderLine(renderer, cx - dx, py, cx + dx, py);
    }
}

void __Draw__::DrawRoundedRect(float x, float y, float w, float h, float radius, std::string hexColor, bool inside) {
    // Ensure the radius does not exceed half of the width or height
    if (radius > w / 2.0f) radius = w / 2.0f;
    if (radius > h / 2.0f) radius = h / 2.0f;

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    // Number of segments per corner for smoothness
    const int segments = 16;

    // Core inner dimensions
    float innerLeft = x + radius;
    float innerRight = x + w - radius;
    float innerTop = y + radius;
    float innerBottom = y + h - radius;

    RGB rgb = hexStringToRgb(hexColor);
    float r = static_cast<float>(rgb.r);
    float g = static_cast<float>(rgb.g);
    float b = static_cast<float>(rgb.b);

    // Center vertex for the fan-like structure
    SDL_Vertex centerVert;
    centerVert.position = { x + w / 2.0f, y + h / 2.0f };
    centerVert.color = { (inside ? r - 90 : r) / 255.0f,
                         (inside ? g - 90 : g) / 255.0f,
                         (inside ? b - 90 : b) / 255.0f,
                         255 / 255.0f };
    vertices.push_back(centerVert);

    auto addCorner = [&](float cx, float cy, float startAngle) {
        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (i * (M_PI / 2.0f) / segments);
            SDL_Vertex v;
            v.position.x = cx + radius * cosf(angle);
            v.position.y = cy + radius * sinf(angle);
            v.color = centerVert.color;
            vertices.push_back(v);
        }
        };

    // Generate vertices for all 4 corners clockwise
    addCorner(innerRight, innerBottom, 0.0f);          // Bottom-Right
    addCorner(innerLeft, innerBottom, M_PI / 2.0f);   // Bottom-Left
    addCorner(innerLeft, innerTop, M_PI);          // Top-Left
    addCorner(innerRight, innerTop, 3.0f * M_PI / 2.0f); // Top-Right

    // Build indices to connect the vertices into triangles
    int totalPoints = vertices.size() - 1;
    for (int i = 1; i <= totalPoints; ++i) {
        indices.push_back(0); // Center
        indices.push_back(i);
        indices.push_back((i % totalPoints) + 1);
    }

    // Render the constructed geometry
    SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}


bool __UI__::Button(std::string text, int x, int y, int w, int h, std::string hexColor, int mx, int my, bool mDown) {
    // Check if mouse cursor is inside button boundaries
    bool inside = (mx >= x) && (mx <= x + w) && (my >= y) && (my <= y + h);
    
    Draw.DrawRoundedRect(x, y, w, h, 25.0f, hexColor, inside);
    std::string invHex = invertHexColor(hexColor);
    Text(text, x - 16, y - 66, invHex);

    // Return true only if mouse is inside and clicked
    return inside && mDown;
}

bool __UI__::isClicked(std::uint32_t mouseState) {
    return (mouseState & SDL_BUTTON_LMASK);
}

void __UI__::Text(std::string text, int x, int y, std::string hexColor) {
    const char* txt = text.c_str();
    RGB rgb = hexStringToRgb(hexColor);
    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255); // White text

    SDL_RenderDebugText(renderer, x, y, txt);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}