#include "sdl3_win64.hpp"
#include "internal.hpp"
#include <SDL3/SDL.h>

using namespace sdlint;

void __Draw__::Rect(int x, int y, int w, int h, std::string hexColor) {
	if (!renderer) return;
	RGB rgb = hexStringToRgb(hexColor);
	SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
	SDL_FRect rect{ (float)x, (float)y, (float)w, (float)h };
	SDL_RenderFillRect(renderer, &rect);
}

void __Draw__::RectOutline(int x, int y, int w, int h, std::string hexColor) {
	if (!renderer) return;
	RGB rgb = hexStringToRgb(hexColor);
	SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
	SDL_FRect rect{ (float)x, (float)y, (float)w, (float)h };
	SDL_RenderRect(renderer, &rect);
}

void __Draw__::Line(int x1, int y1, int x2, int y2, std::string hexColor) {
	if (!renderer) return;
	RGB rgb = hexStringToRgb(hexColor);
	SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
	SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2);
}

void __Draw__::Point(int x, int y, std::string hexColor) {
	if (!renderer) return;
	RGB rgb = hexStringToRgb(hexColor);
	SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
	SDL_RenderPoint(renderer, (float)x, (float)y);
}